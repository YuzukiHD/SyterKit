/* SPDX-License-Identifier: GPL-2.0+ */

#include <barrier.h>
#include <io.h>
#include <mmu.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <jmp.h>
#include <log.h>

#include <dt2c/driver.h>
#include <drivers/dram/dram.h>
#include <drivers/rtc/rtc.h>

extern int init_DRAM(int type, void *buff);

void __usdelay(unsigned long us)
{
	udelay(us);
}

int set_ddr_voltage(unsigned int vol_val)
{
	return 0;
}

uint32_t sunxi_dram_init(sunxi_dram_t *dram)
{
	if (dram == NULL || dram->parameter_count == 0U)
		return 0U;
	dram->size = init_DRAM(0, dram->parameters);
	return dram->size;
}

DT2C_DRIVER_COMPAT("allwinner,sunxi-dram");
