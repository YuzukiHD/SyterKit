/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file remoteproc.c
 * @brief Remote processor management and firmware loading.
 *
 * This layer validates a remoteproc descriptor, resolves the entry point from
 * an ELF image, and drives the lifecycle (prepare, load, start, reset, dump)
 * through the SoC-specific operations supplied in the descriptor.
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <types.h>

#include <driver.h>
#include <drivers/remoteproc/remoteproc.h>
#include <lib/elf/elf_loader.h>

/**
 * @brief Validate a remoteproc descriptor.
 *
 * Checks that the descriptor carries an operations table and at least one
 * firmware, that firmware and address-map counts are within limits, and that
 * every address range is ordered, non-wrapping, and does not overlap its
 * predecessor.
 *
 * @param[in] remoteproc Remote processor descriptor to validate.
 * @return DRIVER_OK when valid, DRIVER_ERROR_INVALID otherwise.
 */
static int sunxi_remoteproc_validate(const sunxi_remoteproc_t *remoteproc)
{
	size_t index;

	if (remoteproc == NULL || remoteproc->ops == NULL || remoteproc->firmware_count == 0U || remoteproc->firmware_count > SUNXI_REMOTEPROC_MAX_FIRMWARES ||
	    remoteproc->address_map_count > SUNXI_REMOTEPROC_MAX_ADDRESS_MAPS || remoteproc->register_count > SUNXI_REMOTEPROC_MAX_REGISTERS)
		return DRIVER_ERROR_INVALID;

	for (index = 0U; index < remoteproc->address_map_count; ++index) {
		const sunxi_remoteproc_address_map_t *range = &remoteproc->address_map[index];

		if (range->device_start > range->device_end || range->physical_start > (uintptr_t)-1 - (range->device_end - range->device_start) ||
		    (index != 0U && range->device_start <= remoteproc->address_map[index - 1U].device_end))
			return DRIVER_ERROR_INVALID;
	}
	return DRIVER_OK;
}

/**
 * @brief Resolve the remote processor entry point from its firmware ELF.
 *
 * When the descriptor requests entry-point extraction from an ELF image, this
 * reads the ELF entry address of the first firmware and stores it in the
 * descriptor.
 *
 * @param[in,out] remoteproc Remote processor descriptor to update.
 * @return DRIVER_OK on success, DRIVER_ERROR_INVALID on failure.
 */
static int sunxi_remoteproc_resolve_entry(sunxi_remoteproc_t *remoteproc)
{
	phys_addr_t load_address;

	if (!remoteproc->entry_from_elf)
		return DRIVER_OK;
	if (remoteproc->firmware[0].load_address == 0U)
		return DRIVER_ERROR_INVALID;

	load_address = (phys_addr_t)remoteproc->firmware[0].load_address;
	if (remoteproc->format == SUNXI_REMOTEPROC_FIRMWARE_ELF32)
		remoteproc->entry = (uintptr_t)elf32_get_entry_addr(load_address);
	else if (remoteproc->format == SUNXI_REMOTEPROC_FIRMWARE_ELF64)
		remoteproc->entry = (uintptr_t)elf64_get_entry_addr(load_address);
	else
		return DRIVER_ERROR_INVALID;
	return DRIVER_OK;
}

/**
 * @brief Reset the remote processor.
 *
 * @param[in] remoteproc Remote processor descriptor.
 * @return DRIVER_OK on success, DRIVER_ERROR_INVALID when the descriptor or
 *         the reset operation is unavailable.
 */
int sunxi_remoteproc_reset(sunxi_remoteproc_t *remoteproc)
{
	if (sunxi_remoteproc_validate(remoteproc) != DRIVER_OK || remoteproc->ops->reset == NULL)
		return DRIVER_ERROR_INVALID;
	return remoteproc->ops->reset(remoteproc);
}

/**
 * @brief Load an ELF firmware image into the remote processor.
 *
 * Builds the address-map remapping table, resolves the entry point, and loads
 * either an ELF32 image through the remapping path or an ELF64 image directly.
 *
 * @param[in,out] remoteproc Remote processor descriptor.
 * @return DRIVER_OK on success, DRIVER_ERROR_INVALID on any failure.
 */
int sunxi_remoteproc_load(sunxi_remoteproc_t *remoteproc)
{
	phys_addr_t load_address;
	vaddr_range_t ranges[SUNXI_REMOTEPROC_MAX_ADDRESS_MAPS];
	vaddr_map_t mapping;
	size_t index;

	if (sunxi_remoteproc_validate(remoteproc) != DRIVER_OK || remoteproc->firmware[0].load_address == 0U || remoteproc->format == SUNXI_REMOTEPROC_FIRMWARE_RAW ||
	    sunxi_remoteproc_resolve_entry(remoteproc) != DRIVER_OK)
		return DRIVER_ERROR_INVALID;

	load_address = (phys_addr_t)remoteproc->firmware[0].load_address;
	for (index = 0U; index < remoteproc->address_map_count; ++index) {
		ranges[index].vstart = remoteproc->address_map[index].device_start;
		ranges[index].vend = remoteproc->address_map[index].device_end;
		ranges[index].pstart = remoteproc->address_map[index].physical_start;
	}
	mapping.range = ranges;
	mapping.range_size = (uint32_t)remoteproc->address_map_count;

	if (remoteproc->format == SUNXI_REMOTEPROC_FIRMWARE_ELF32) {
		return load_elf32_image_remap(load_address, &mapping) == 0 ? DRIVER_OK : DRIVER_ERROR_INVALID;
	}
	if (remoteproc->format == SUNXI_REMOTEPROC_FIRMWARE_ELF64) {
		if (remoteproc->address_map_count != 0U)
			return DRIVER_ERROR_INVALID;
		return load_elf64_image(load_address) == 0 ? DRIVER_OK : DRIVER_ERROR_INVALID;
	}
	return DRIVER_ERROR_INVALID;
}

/**
 * @brief Copy a raw firmware image into the remoteproc address maps.
 *
 * Walks each address-map range, clipping the copy to the range boundaries and
 * the firmware size, and memcpy's the payload to the physical destination.
 *
 * @param[in,out] remoteproc Remote processor descriptor.
 * @param[in] firmware Pointer to the raw firmware image.
 * @param[in] size Size of the firmware image in bytes.
 * @return DRIVER_OK when the whole image was copied, DRIVER_ERROR_INVALID
 *         otherwise.
 */
static int sunxi_remoteproc_copy_raw(sunxi_remoteproc_t *remoteproc, const void *firmware, size_t size)
{
	const uint8_t *source = firmware;
	size_t copied = 0U;
	size_t index;

	if (remoteproc->format != SUNXI_REMOTEPROC_FIRMWARE_RAW || remoteproc->address_map_count == 0U)
		return DRIVER_ERROR_INVALID;

	for (index = 0U; index < remoteproc->address_map_count; ++index) {
		const sunxi_remoteproc_address_map_t *range = &remoteproc->address_map[index];
		size_t length;

		if (range->device_start >= size)
			continue;
		length = size - range->device_start;
		if (range->device_end - range->device_start < length - 1U)
			length = range->device_end - range->device_start + 1U;
		if (copied > (size_t)-1 - length)
			return DRIVER_ERROR_INVALID;
		memcpy((void *)range->physical_start, source + range->device_start, length);
		copied += length;
	}
	return copied == size ? DRIVER_OK : DRIVER_ERROR_INVALID;
}

/**
 * @brief Load a firmware buffer into the remote processor.
 *
 * Validates the firmware payload size against the first firmware region and
 * forwards the load to the optional SoC load_buffer operation, falling back to
 * a raw copy when the operation is not provided.
 *
 * @param[in,out] remoteproc Remote processor descriptor.
 * @param[in] firmware Pointer to the firmware image.
 * @param[in] size Size of the firmware image in bytes.
 * @return DRIVER_OK on success, DRIVER_ERROR_INVALID on any failure.
 */
int sunxi_remoteproc_load_buffer(sunxi_remoteproc_t *remoteproc, const void *firmware, size_t size)
{
	if (sunxi_remoteproc_validate(remoteproc) != DRIVER_OK || firmware == NULL || size == 0U || size > remoteproc->firmware[0].region_size)
		return DRIVER_ERROR_INVALID;
	if (remoteproc->ops->load_buffer != NULL)
		return remoteproc->ops->load_buffer(remoteproc, firmware, size);
	return sunxi_remoteproc_copy_raw(remoteproc, firmware, size);
}

/**
 * @brief Prepare the remote processor for operation.
 *
 * Resolves the entry point and invokes the optional SoC prepare operation.
 *
 * @param[in,out] remoteproc Remote processor descriptor.
 * @return DRIVER_OK on success, DRIVER_ERROR_INVALID on any failure.
 */
int sunxi_remoteproc_prepare(sunxi_remoteproc_t *remoteproc)
{
	if (sunxi_remoteproc_validate(remoteproc) != DRIVER_OK || sunxi_remoteproc_resolve_entry(remoteproc) != DRIVER_OK)
		return DRIVER_ERROR_INVALID;
	return remoteproc->ops->prepare == NULL ? DRIVER_OK : remoteproc->ops->prepare(remoteproc);
}

/**
 * @brief Start the remote processor.
 *
 * @param[in] remoteproc Remote processor descriptor.
 * @return DRIVER_OK on success, DRIVER_ERROR_INVALID when the descriptor or
 *         the start operation is unavailable.
 */
int sunxi_remoteproc_start(sunxi_remoteproc_t *remoteproc)
{
	if (sunxi_remoteproc_validate(remoteproc) != DRIVER_OK || remoteproc->ops->start == NULL)
		return DRIVER_ERROR_INVALID;
	return remoteproc->ops->start(remoteproc);
}

/**
 * @brief Dump remote processor state.
 *
 * Invokes the optional SoC dump operation when the descriptor is valid.
 *
 * @param[in] remoteproc Remote processor descriptor.
 */
void sunxi_remoteproc_dump(const sunxi_remoteproc_t *remoteproc)
{
	if (sunxi_remoteproc_validate(remoteproc) == DRIVER_OK && remoteproc->ops->dump != NULL)
		remoteproc->ops->dump(remoteproc);
}
