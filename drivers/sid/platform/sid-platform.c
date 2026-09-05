/* SPDX-License-Identifier: GPL-2.0+ */
#define pr_fmt(fmt) "sid: " fmt

/**
 * @file sid-platform.c
 * @brief Default SID platform eFuse register access.
 *
 * Implements the register-based eFuse read/write sequence shared by the
 * legacy SID controllers.  Platforms with a different register interface
 * (for example the Sun252iw2) provide their own strong definitions that
 * override these weak helpers.  Offsets and masks are taken from the SID
 * control registers.
 */

#include <stdbool.h>
#include <stdint.h>

#include <io.h>

#include <drivers/sid/sid.h>

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
