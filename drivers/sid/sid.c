/* SPDX-License-Identifier: GPL-2.0+ */
#define pr_fmt(fmt) "sid: " fmt

/**
 * @file sid.c
 * @brief SID / efuse core driver.
 *
 * Provides SRAM-backed efuse reads and a textual dump of the known SID
 * sections.  The register-based efuse read/write sequence and the per-SoC
 * section layouts are platform-dependent and live under platform/.
 */

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include <io.h>
#include <log.h>

#include <drivers/sid/sid.h>
#include <dt2c/driver.h>

#include "platform/sid-platform.h"

/**
 * @brief Compute the SRAM-mapped address for a SID offset.
 *
 * @param[in] sid SID controller descriptor.
 * @param[in] offset Byte offset into the SID SRAM window.
 * @param[out] address Receives the computed SRAM address on success.
 * @return true when the offset is valid and the address does not wrap.
 */
static bool sunxi_sid_sram_address(const sunxi_sid_t *sid, uint32_t offset, uintptr_t *address)
{
	uintptr_t sram_base;

	if (address == NULL || sid == NULL || sid->base == 0U || (sid->base & (sizeof(uint32_t) - 1U)) != 0U ||
		(offset & (sizeof(uint32_t) - 1U)) != 0U || sid->size < SUNXI_SID_SRAM_OFFSET + sizeof(uint32_t) ||
		offset > sid->size - SUNXI_SID_SRAM_OFFSET - sizeof(uint32_t))
		return false;

	sram_base = sid->sram_base;
	if (sram_base == 0U) {
		sram_base = sid->base + SUNXI_SID_SRAM_OFFSET;
		if (sram_base < sid->base)
			return false;
	}
	if ((sram_base & (sizeof(uint32_t) - 1U)) != 0U)
		return false;

	*address = sram_base + offset;
	if (*address < sram_base)
		return false;
	return true;
}

/**
 * @brief Read a 32-bit word from the SID SRAM window.
 *
 * @param[in] sid SID controller descriptor.
 * @param[in] offset Byte offset into the SID SRAM window.
 * @return The read word, or zero when the offset is invalid.
 */
uint32_t sunxi_efuse_sram_read(const sunxi_sid_t *sid, uint32_t offset)
{
	uintptr_t address;

	if (!sunxi_sid_sram_address(sid, offset, &address))
		return 0U;
	return read32(address);
}

/**
 * @brief Dump all known SID sections to the console.
 *
 * Prints each section name, offset, and bit width followed by its 32-bit
 * SRAM-read values in a compact table.
 *
 * @param[in] sid SID controller descriptor.
 */
void sunxi_efuse_dump(const sunxi_sid_t *sid)
{
	if (sid == NULL)
		return;

	for (size_t section = 0; section < sunxi_sid_section_count; section++) {
		const sunxi_sid_section_t *entry = &sunxi_sid_sections[section];
		size_t count = (entry->size_bits + 31U) / 32U;

		printk(LOG_LEVEL_MUTE, "%s:(0x%04x %d-bits)", entry->name, entry->offset, entry->size_bits);
		for (size_t index = 0; index < count; index++) {
			if (index % 8U == 0U)
				printk(LOG_LEVEL_MUTE, "\n%-4s", "");
			printk(LOG_LEVEL_MUTE, "%08x ",
				sunxi_efuse_sram_read(sid, entry->offset + index * sizeof(uint32_t)));
		}
		printk(LOG_LEVEL_MUTE, "\n");
	}
}

DT2C_DRIVER_COMPAT("allwinner,sunxi-sid");
