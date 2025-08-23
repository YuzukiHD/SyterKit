/* SPDX-License-Identifier: GPL-2.0+ */

#include <barrier.h>
#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>

#include <common.h>

#include "sys-sid.h"

const struct sid_section_t {
	char *name;
	uint32_t offset;
	uint32_t size_bits;
} sids[] = {
		{"chipid", 0x0000, 128},
		{"brom-conf-try", 0x0010, 32},
		{"thermal-sensor", 0x0014, 64},
		{"ft-zone", 0x001c, 128},
		{"reserved1", 0x002c, 96},
		{"write-protect", 0x0038, 32},
		{"read-protect", 0x003c, 32},
		{"lcjs", 0x0040, 32},
		{"reserved2", 0x0044, 800},
		{"rotpk", 0x00a8, 256},
		{"reserved3", 0x00c8, 448},
};

enum {
	SID_PRCTL = SUNXI_SID_BASE + 0x040,
	SID_PRKEY = SUNXI_SID_BASE + 0x050,
	SID_RDKEY = SUNXI_SID_BASE + 0x060,
	EFUSE_HV_SWITCH = SUNXI_RTC_BASE + 0x204,
};

uint32_t syter_efuse_read(uint32_t offset) {
	uint32_t val;

	val = read32(SID_PRCTL);
	val &= ~((0x1ff << 16) | 0x3);
	val |= offset << 16;
	write32(SID_PRCTL, val);
	val &= ~((0xff << 8) | 0x3);
	val |= (0xac << 8) | 0x2;
	write32(SID_PRCTL, val);
	while (read32(SID_PRCTL) & 0x2)
		;
	val &= ~((0x1ff << 16) | (0xff << 8) | 0x3);
	write32(SID_PRCTL, val);
	val = read32(SID_RDKEY);

	return val;
}

void syter_efuse_write(uint32_t offset, uint32_t value) {
	uint32_t val;

	write32(EFUSE_HV_SWITCH, 0x1);
	write32(SID_PRKEY, value);
	val = read32(SID_PRCTL);
	val &= ~((0x1ff << 16) | 0x3);
	val |= offset << 16;
	write32(SID_PRCTL, val);
	val &= ~((0xff << 8) | 0x3);
	val |= (0xac << 8) | 0x1;
	write32(SID_PRCTL, val);
	while (read32(SID_PRCTL) & 0x1)
		;
	val &= ~((0x1ff << 16) | (0xff << 8) | 0x3);
	write32(SID_PRCTL, val);
	write32(EFUSE_HV_SWITCH, 0x0);
}

void syter_efuse_dump(void) {
	uint32_t buffer[2048 / 4];

	for (int n = 0; n < ARRAY_SIZE(sids); n++) {
		uint32_t count = sids[n].size_bits / 32;
		for (int i = 0; i < count; i++) buffer[i] = syter_efuse_read(sids[n].offset + i * 4);

		printk(LOG_LEVEL_MUTE, "%s:(0x%04x %d-bits)", sids[n].name, sids[n].offset, sids[n].size_bits);
		for (int i = 0; i < count; i++) {
			if (i >= 0 && ((i % 8) == 0))
				printk(LOG_LEVEL_MUTE, "\n%-4s", "");
			printk(LOG_LEVEL_MUTE, "%08x ", buffer[i]);
		}
		printk(LOG_LEVEL_MUTE, "\n");
	}
}