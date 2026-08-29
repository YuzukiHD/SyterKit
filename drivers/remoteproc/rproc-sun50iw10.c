/* SPDX-License-Identifier: GPL-2.0+ */
#define pr_fmt(fmt) "rproc-sun50iw10: " fmt

/**
 * @file rproc-sun50iw10.c
 * @brief Sun50iw10 AR100 remote processor load helper.
 *
 * Implements the remoteproc load callback that copies firmware into the
 * AR100 memory map and starts the core, guarding against re-flashing on
 * warm boots through RTC-backed cold-start tracking.
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <driver.h>
#include <drivers/remoteproc/remoteproc.h>
#include <drivers/rtc/rtc.h>
#include <dt2c/driver.h>
#include <io.h>
#include <log.h>

#define SUN50IW10_RTC_DATA_COLD_START 7

/**
 * @brief Register resource indices used by the AR100 remote processor.
 */
enum sun50iw10_ar100_register {
	SUN50IW10_AR100_SYSCTRL,
	SUN50IW10_AR100_RCPU_CFG,
};

/**
 * @brief Test whether an SoC chip ID is affected by the cold-start quirk.
 *
 * @param[in] id SoC ID read from the system controller.
 * @return True if the chip ID requires the cold-start handling, false otherwise.
 */
static bool sun50iw10_ar100_affected(uint32_t id)
{
	return id == 0U || id == 3U || id == 4U || id == 5U;
}

/**
 * @brief Copy firmware into the AR100 address map.
 *
 * Walks the remote processor address map and copies the firmware ranges
 * into their physical destinations, validating each range before writing.
 *
 * @param[in] remoteproc Remote processor descriptor holding the address map.
 * @param[in] firmware Pointer to the firmware image to load.
 * @param[in] size Size of the firmware image in bytes.
 * @return DRIVER_OK when the whole image was copied, DRIVER_ERROR_INVALID on
 *         any range error.
 */
static int sun50iw10_ar100_copy(const sunxi_remoteproc_t *remoteproc, const void *firmware, size_t size)
{
	const uint8_t *source = firmware;
	size_t copied = 0U;
	size_t index;

	for (index = 0U; index < remoteproc->address_map_count; ++index) {
		const sunxi_remoteproc_address_map_t *range = &remoteproc->address_map[index];
		size_t length;

		if (range->device_start >= size)
			continue;
		if (range->device_start > range->device_end || range->physical_start > (uintptr_t)-1 - (range->device_end - range->device_start))
			return DRIVER_ERROR_INVALID;
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
 * @brief Load a firmware buffer into the AR100 and start it.
 *
 * On a cold start the firmware is copied into the AR100 memory map and the
 * core is released; on a warm start the copy is skipped and the already
 * loaded firmware is reused. The function then parks the primary core.
 *
 * @param[in] remoteproc Remote processor descriptor holding the register
 *                       bases and the address map.
 * @param[in] firmware Pointer to the firmware image to load.
 * @param[in] size Size of the firmware image in bytes.
 * @return DRIVER_OK on success, or DRIVER_ERROR_INVALID on failure.
 */
static int sun50iw10_ar100_load_buffer(sunxi_remoteproc_t *remoteproc, const void *firmware, size_t size)
{
	uint32_t cold_start;
	uint32_t id;
	uint32_t value;
	uintptr_t sysctrl = remoteproc->registers[SUN50IW10_AR100_SYSCTRL].base;
	uintptr_t rcpu_cfg = remoteproc->registers[SUN50IW10_AR100_RCPU_CFG].base;

	id = readl(sysctrl + 0x24U) & 0x07U;
	if (remoteproc->rtc == NULL)
		return DRIVER_ERROR_INVALID;
	cold_start = rtc_read_data(remoteproc->rtc, SUN50IW10_RTC_DATA_COLD_START);
	pr_debug("soc-id=%u cold-start=%u\n", id, cold_start);
	if (!sun50iw10_ar100_affected(id))
		return DRIVER_OK;
	if (cold_start != 0U) {
		rtc_write_data(remoteproc->rtc, SUN50IW10_RTC_DATA_COLD_START, 0U);
		return DRIVER_OK;
	}

	rtc_write_data(remoteproc->rtc, SUN50IW10_RTC_DATA_COLD_START, 1U);
	value = readl(rcpu_cfg);
	value &= ~1U;
	writel(value, rcpu_cfg);
	if (sun50iw10_ar100_copy(remoteproc, firmware, size) != DRIVER_OK)
		return DRIVER_ERROR_INVALID;
	asm volatile("dsb" ::: "memory");

	value = readl(rcpu_cfg);
	value &= ~1U;
	writel(value, rcpu_cfg);
	value = readl(rcpu_cfg);
	value |= 1U;
	writel(value, rcpu_cfg);
	for (;;)
		asm volatile("wfi");
}

/**
 * @brief AR100 remote processor operations exposed to the remoteproc core.
 */
const sunxi_remoteproc_ops_t sunxi_remoteproc_ops = {
	.load_buffer = sun50iw10_ar100_load_buffer,
};

DT2C_DRIVER_COMPAT("allwinner,sun50iw10-ar100");
