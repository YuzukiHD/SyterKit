/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file sid-sun252iw2.c
 * @brief Sun252iw2 eFuse SID driver.
 *
 * Implements eFuse read and write access through the SID controller and
 * defines the named eFuse storage sections available on this SoC.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <io.h>

#include <drivers/sid/sid.h>

#include "sid-internal.h"

#define SID_PRCTL_OFFSET 0x00U
#define SID_PR_ADDR_OFFSET 0x04U
#define SID_PRKEY_OFFSET 0x08U
#define SID_RDKEY_OFFSET 0x0CU
#define SID_KEY_MASK 0xffffU
#define SID_OPERATION_MASK 0x3U
#define SID_READ_KEY 0xadbfU
#define SID_WRITE_KEY 0xe0c9U
#define SID_OFFSET_MASK 0x1ffU
#define SID_OPERATION_RETRIES 1000000U

/**
 * @brief Validate a byte offset against the SID controller eFuse window.
 *
 * Checks that the SID descriptor is present, that the eFuse window is large
 * enough for a full word, that the offset is word-aligned and that it lies
 * within the SID offset mask.
 *
 * @param[in] sid SID controller description.
 * @param[in] offset Word-aligned eFuse byte offset.
 * @return True when the offset is usable, false otherwise.
 */
static bool sunxi_sid_offset_valid(const sunxi_sid_t *sid, uint32_t offset)
{
	return sid != NULL && sid->base != 0U && sid->size >= SID_RDKEY_OFFSET + sizeof(uint32_t) &&
	       (offset & (sizeof(uint32_t) - 1U)) == 0U && offset <= SID_OFFSET_MASK;
}

/**
 * @brief Read a 32-bit value from the eFuse SRAM window.
 *
 * Programs the SID controller to fetch the word at the given offset and
 * returns it once the read operation completes.
 *
 * @param[in] sid SID controller description.
 * @param[in] offset Word-aligned eFuse byte offset.
 * @return The 32-bit value read from eFuse, or 0 on invalid offset or
 *         operation timeout.
 */
uint32_t sunxi_efuse_read(const sunxi_sid_t *sid, uint32_t offset)
{
	uintptr_t prctl;
	uint32_t value;
	bool timed_out = false;
	uint32_t retries = SID_OPERATION_RETRIES;

	if (!sunxi_sid_offset_valid(sid, offset))
		return 0U;

	write32(sid->base + SID_PR_ADDR_OFFSET, offset / sizeof(uint32_t));
	prctl = sid->base + SID_PRCTL_OFFSET;
	value = read32(prctl);
	value &= ~((SID_KEY_MASK << 16) | SID_OPERATION_MASK);
	value |= (SID_READ_KEY << 16) | 0x2U;
	write32(prctl, value);
	while (read32(prctl) & 0x2U) {
		if (retries-- == 0U) {
			timed_out = true;
			break;
		}
	}

	value &= ~((SID_KEY_MASK << 16) | SID_OPERATION_MASK);
	write32(prctl, value);
	if (timed_out)
		return 0U;
	return read32(sid->base + SID_RDKEY_OFFSET);
}

/**
 * @brief Write a 32-bit value to an eFuse word.
 *
 * Powers up the eFuse high-voltage switch, programs the write key and data,
 * and waits for the write operation to finish before powering the switch
 * down again.
 *
 * @param[in] sid SID controller description.
 * @param[in] offset Word-aligned eFuse byte offset.
 * @param[in] value 32-bit value to program into the eFuse.
 * @return 0 on success, or -1 on invalid arguments or operation timeout.
 */
int sunxi_efuse_write(const sunxi_sid_t *sid, uint32_t offset, uint32_t value)
{
	uintptr_t prctl;
	uint32_t control;
	uint32_t retries = SID_OPERATION_RETRIES;

	if (!sunxi_sid_offset_valid(sid, offset) || sid->efuse_hv_switch == 0U)
		return -1;

	write32(sid->efuse_hv_switch, 0x1U);
	write32(sid->base + SID_PRKEY_OFFSET, value);
	write32(sid->base + SID_PR_ADDR_OFFSET, offset / sizeof(uint32_t));
	prctl = sid->base + SID_PRCTL_OFFSET;
	control = read32(prctl);
	control &= ~((SID_KEY_MASK << 16) | SID_OPERATION_MASK);
	control |= (SID_WRITE_KEY << 16) | 0x1U;
	write32(prctl, control);
	while (read32(prctl) & 0x1U) {
		if (retries-- == 0U) {
			control &= ~((SID_KEY_MASK << 16) | SID_OPERATION_MASK);
			write32(prctl, control);
			write32(sid->efuse_hv_switch, 0x0U);
			return -1;
		}
	}

	control &= ~((SID_KEY_MASK << 16) | SID_OPERATION_MASK);
	write32(prctl, control);
	write32(sid->efuse_hv_switch, 0x0U);
	return 0;
}

/**
 * @brief eFuse section layout for the Sun252iw2 SoC.
 *
 * Each entry names an eFuse region, its byte offset and its size in bits.
 */
const sunxi_sid_section_t sunxi_sid_sections[] = {
	{ "chipid", 0x0000, 128 },
	{ "brom_conf", 0x0010, 32 },
	{ "thermal", 0x0014, 32 },
	{ "res", 0x0018, 32 },
	{ "sec_ctrl", 0x001c, 32 },
	{ "ssk", 0x0020, 128 },
	{ "oem1", 0x0030, 96 },
	{ "ft_zone", 0x003c, 32 },
};

/**
 * @brief Number of entries in #sunxi_sid_sections.
 */
const size_t sunxi_sid_section_count = sizeof(sunxi_sid_sections) / sizeof(sunxi_sid_sections[0]);
