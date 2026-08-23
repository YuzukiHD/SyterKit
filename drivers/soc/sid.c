/* SPDX-License-Identifier: GPL-2.0+ */

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include <io.h>
#include <log.h>

#include <drivers/soc/sid.h>
#include <dt2c/driver.h>

#include "sid-internal.h"

#define SID_PRCTL_OFFSET 0x040U
#define SID_PRKEY_OFFSET 0x050U
#define SID_RDKEY_OFFSET 0x060U
#define SID_SRAM_OFFSET 0x200U
#define SID_OFFSET_MASK 0x1ffU
#define SID_OPERATION_MASK 0x3U
#define SID_KEY_MASK 0xffU
#define SID_ACCESS_KEY 0xacU
#define SID_OPERATION_RETRIES 1000000U

static bool sunxi_sid_offset_valid(const sunxi_sid_t *sid, uint32_t offset) {
	return sid != NULL && sid->base != 0U &&
	       sid->size >= SID_RDKEY_OFFSET + sizeof(uint32_t) &&
	       (offset & (sizeof(uint32_t) - 1U)) == 0U &&
	       offset <= SID_OFFSET_MASK;
}

uint32_t sunxi_sid_read_sram(const sunxi_sid_t *sid, uint32_t offset) {
	uintptr_t address;
	uintptr_t sram_base;

	if (sid == NULL || sid->base == 0U ||
	    (sid->base & (sizeof(uint32_t) - 1U)) != 0U ||
	    (offset & (sizeof(uint32_t) - 1U)) != 0U ||
	    sid->size < SID_SRAM_OFFSET + sizeof(uint32_t) ||
	    offset > sid->size - SID_SRAM_OFFSET - sizeof(uint32_t))
		return 0U;

	sram_base = sid->base + SID_SRAM_OFFSET;
	if (sram_base < sid->base)
		return 0U;
	address = sram_base + offset;
	if (address < sram_base)
		return 0U;
	return read32(address);
}

uint32_t sunxi_efuse_read(const sunxi_sid_t *sid, uint32_t offset) {
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

	value &= ~((SID_OFFSET_MASK << 16) | (SID_KEY_MASK << 8) |
		   SID_OPERATION_MASK);
	write32(prctl, value);
	if (timed_out)
		return 0U;
	return read32(sid->base + SID_RDKEY_OFFSET);
}

int sunxi_efuse_write(const sunxi_sid_t *sid, uint32_t offset,
		      uint32_t value) {
	uintptr_t prctl;
	uint32_t control;
	uint32_t retries = SID_OPERATION_RETRIES;

	if (!sunxi_sid_offset_valid(sid, offset) ||
	    sid->efuse_hv_switch == 0U)
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
			control &= ~((SID_OFFSET_MASK << 16) |
				     (SID_KEY_MASK << 8) | SID_OPERATION_MASK);
			write32(prctl, control);
			write32(sid->efuse_hv_switch, 0x0U);
			return -1;
		}
	}

	control &= ~((SID_OFFSET_MASK << 16) | (SID_KEY_MASK << 8) |
		     SID_OPERATION_MASK);
	write32(prctl, control);
	write32(sid->efuse_hv_switch, 0x0U);
	return 0;
}

void sunxi_efuse_dump(const sunxi_sid_t *sid) {
	if (sid == NULL)
		return;

	for (size_t section = 0; section < sunxi_sid_section_count; section++) {
		const sunxi_sid_section_t *entry = &sunxi_sid_sections[section];
		size_t count = (entry->size_bits + 31U) / 32U;

		printk(LOG_LEVEL_MUTE, "%s:(0x%04x %d-bits)", entry->name,
		       entry->offset, entry->size_bits);
		for (size_t index = 0; index < count; index++) {
			if (index % 8U == 0U)
				printk(LOG_LEVEL_MUTE, "\n%-4s", "");
			printk(LOG_LEVEL_MUTE, "%08x ", sunxi_efuse_read(
					sid, entry->offset +
					     index * sizeof(uint32_t)));
		}
		printk(LOG_LEVEL_MUTE, "\n");
	}
}

DT2C_DRIVER_COMPAT("allwinner,sunxi-sid");
