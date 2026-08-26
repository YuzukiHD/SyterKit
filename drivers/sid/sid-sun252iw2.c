/* SPDX-License-Identifier: GPL-2.0+ */

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

static bool sunxi_sid_offset_valid(const sunxi_sid_t *sid, uint32_t offset)
{
	return sid != NULL && sid->base != 0U && sid->size >= SID_RDKEY_OFFSET + sizeof(uint32_t) &&
	       (offset & (sizeof(uint32_t) - 1U)) == 0U && offset <= SID_OFFSET_MASK;
}

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

const size_t sunxi_sid_section_count = sizeof(sunxi_sid_sections) / sizeof(sunxi_sid_sections[0]);
