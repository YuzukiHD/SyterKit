/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file sid.c
 * @brief SID / efuse register access and dump helpers.
 *
 * Provides SRAM-backed efuse reads, the register-based efuse read/write
 * sequence with the correct operation key, and a textual dump of the known
 * SID sections.  Offsets and masks are taken from the SID control registers.
 */

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include <io.h>
#include <log.h>

#include <drivers/sid/sid.h>
#include <dt2c/driver.h>

#include "sid-internal.h"

#define SID_PRCTL_OFFSET      0x040U
#define SID_PRKEY_OFFSET      0x050U
#define SID_RDKEY_OFFSET      0x060U
#define SID_OFFSET_MASK	      0x1ffU
#define SID_OPERATION_MASK    0x3U
#define SID_KEY_MASK	      0xffU
#define SID_ACCESS_KEY	      0xacU
#define SID_OPERATION_RETRIES 1000000U

/**
 * @brief Check that a SID offset is addressable through the register window.
 *
 * @param[in] sid SID controller descriptor.
 * @param[in] offset Byte offset relative to the SID base.
 * @return true when the offset is word-aligned and within the register window.
 */
static bool sunxi_sid_offset_valid(const sunxi_sid_t *sid, uint32_t offset)
{
	return sid != NULL && sid->base != 0U && sid->size >= SID_RDKEY_OFFSET + sizeof(uint32_t) &&
	       (offset & (sizeof(uint32_t) - 1U)) == 0U && offset <= SID_OFFSET_MASK;
}

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
 * @brief Read a 32-bit efuse word through the SID controller registers.
 *
 * Programs the PRCTL offset and access key, polls until the read operation
 * completes, and returns the value latched in the RDKEY register.
 *
 * @param[in] sid SID controller descriptor.
 * @param[in] offset Word offset of the efuse value to read.
 * @return The efuse word, or zero on timeout or an invalid offset.
 */
uint32_t __attribute__((weak)) sunxi_efuse_read(const sunxi_sid_t *sid, uint32_t offset)
{
	uintptr_t prctl;
	uint32_t value;
	bool timed_out = false;
	uint32_t retries = SID_OPERATION_RETRIES;

	if (!sunxi_sid_offset_valid(sid, offset))
		return 0U;

	prctl = sid->base + SID_PRCTL_OFFSET;
	value = read32(prctl);
	value &= ~((SID_OFFSET_MASK << 16) | SID_OPERATION_MASK);
	value |= offset << 16;
	write32(prctl, value);

	value &= ~((SID_KEY_MASK << 8) | SID_OPERATION_MASK);
	value |= (SID_ACCESS_KEY << 8) | 0x2U;
	write32(prctl, value);
	while (read32(prctl) & 0x2U) {
		if (retries-- == 0U) {
			timed_out = true;
			break;
		}
	}

	value &= ~((SID_OFFSET_MASK << 16) | (SID_KEY_MASK << 8) | SID_OPERATION_MASK);
	write32(prctl, value);
	if (timed_out)
		return 0U;
	return read32(sid->base + SID_RDKEY_OFFSET);
}

/**
 * @brief Write a 32-bit efuse word through the SID controller registers.
 *
 * Raises the high-voltage switch, programs the PRKEY data and PRCTL offset,
 * starts the write operation, and polls for completion before restoring the
 * high-voltage switch.
 *
 * @param[in] sid SID controller descriptor.
 * @param[in] offset Word offset of the efuse value to write.
 * @param[in] value Value to program into the efuse.
 * @return 0 on success, -1 on timeout or an invalid offset.
 */
int __attribute__((weak)) sunxi_efuse_write(const sunxi_sid_t *sid, uint32_t offset, uint32_t value)
{
	uintptr_t prctl;
	uint32_t control;
	uint32_t retries = SID_OPERATION_RETRIES;

	if (!sunxi_sid_offset_valid(sid, offset) || sid->efuse_hv_switch == 0U)
		return -1;

	write32(sid->efuse_hv_switch, 0x1U);
	write32(sid->base + SID_PRKEY_OFFSET, value);
	prctl = sid->base + SID_PRCTL_OFFSET;
	control = read32(prctl);
	control &= ~((SID_OFFSET_MASK << 16) | SID_OPERATION_MASK);
	control |= offset << 16;
	write32(prctl, control);

	control &= ~((SID_KEY_MASK << 8) | SID_OPERATION_MASK);
	control |= (SID_ACCESS_KEY << 8) | 0x1U;
	write32(prctl, control);
	while (read32(prctl) & 0x1U) {
		if (retries-- == 0U) {
			control &= ~((SID_OFFSET_MASK << 16) | (SID_KEY_MASK << 8) | SID_OPERATION_MASK);
			write32(prctl, control);
			write32(sid->efuse_hv_switch, 0x0U);
			return -1;
		}
	}

	control &= ~((SID_OFFSET_MASK << 16) | (SID_KEY_MASK << 8) | SID_OPERATION_MASK);
	write32(prctl, control);
	write32(sid->efuse_hv_switch, 0x0U);
	return 0;
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
