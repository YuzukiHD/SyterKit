/* SPDX-License-Identifier: GPL-2.0+ */

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <types.h>

#include <driver.h>
#include <drivers/remoteproc/remoteproc.h>
#include <lib/elf/elf_loader.h>

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

int sunxi_remoteproc_reset(sunxi_remoteproc_t *remoteproc)
{
	if (sunxi_remoteproc_validate(remoteproc) != DRIVER_OK || remoteproc->ops->reset == NULL)
		return DRIVER_ERROR_INVALID;
	return remoteproc->ops->reset(remoteproc);
}

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

int sunxi_remoteproc_load_buffer(sunxi_remoteproc_t *remoteproc, const void *firmware, size_t size)
{
	if (sunxi_remoteproc_validate(remoteproc) != DRIVER_OK || firmware == NULL || size == 0U || size > remoteproc->firmware[0].region_size)
		return DRIVER_ERROR_INVALID;
	if (remoteproc->ops->load_buffer != NULL)
		return remoteproc->ops->load_buffer(remoteproc, firmware, size);
	return sunxi_remoteproc_copy_raw(remoteproc, firmware, size);
}

int sunxi_remoteproc_prepare(sunxi_remoteproc_t *remoteproc)
{
	if (sunxi_remoteproc_validate(remoteproc) != DRIVER_OK || sunxi_remoteproc_resolve_entry(remoteproc) != DRIVER_OK)
		return DRIVER_ERROR_INVALID;
	return remoteproc->ops->prepare == NULL ? DRIVER_OK : remoteproc->ops->prepare(remoteproc);
}

int sunxi_remoteproc_start(sunxi_remoteproc_t *remoteproc)
{
	if (sunxi_remoteproc_validate(remoteproc) != DRIVER_OK || remoteproc->ops->start == NULL)
		return DRIVER_ERROR_INVALID;
	return remoteproc->ops->start(remoteproc);
}

void sunxi_remoteproc_dump(const sunxi_remoteproc_t *remoteproc)
{
	if (sunxi_remoteproc_validate(remoteproc) == DRIVER_OK && remoteproc->ops->dump != NULL)
		remoteproc->ops->dump(remoteproc);
}
