/* SPDX-License-Identifier: Apache-2.0 */

#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <barrier.h>
#include <types.h>
#include <io.h>

#include <log.h>

#include "sys-sid.h"

uint32_t efuse_read(uint32_t offset)
{
	uint32_t val;

	val = read32(SID_PRCTL);
	val &= ~((0x1ff << 16) | 0x3);
	val |= offset << 16;
	write32(SID_PRCTL, val);
	val &= ~((0xff << 8) | 0x3);
	val |= (0xac << 8) | 0x2;
	write32(SID_PRCTL, val);
	while(read32(SID_PRCTL) & 0x2);
	val &= ~((0x1ff << 16) | (0xff << 8) | 0x3);
	write32(SID_PRCTL, val);
	val = read32(SID_RDKEY);

	return val;
}

void efuse_write(uint32_t offset, uint32_t value)
{
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
	while(read32(SID_PRCTL) & 0x1);
	val &= ~((0x1ff << 16) | (0xff << 8) | 0x3);
	write32(SID_PRCTL, val);
	write32(EFUSE_HV_SWITCH, 0x0);
}

void dump_efuse(void)
{
    uint32_t buffer[2048 / 4];

	for (int n = 0; n < ARRAY_SIZE(sids); n++)
	{
	    uint32_t count = sids[n].size_bits / 32;
		for (int i = 0; i < count; i++)
			buffer[i] = efuse_read(sids[n].offset + i * 4);
		    
        printk(LOG_LEVEL_MUTE, "%s:(0x%04x %d-bits)", sids[n].name, sids[n].offset, sids[n].size_bits);
		for (int i = 0; i < count; i++)
		{
			if(i >= 0 && ((i % 8) == 0))
				printk(LOG_LEVEL_MUTE, "\r\n%-4s", "");
			printk(LOG_LEVEL_MUTE, "%08x ", buffer[i]);
		}
		printk(LOG_LEVEL_MUTE, "\r\n");
	}
}