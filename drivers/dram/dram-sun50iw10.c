/* SPDX-License-Identifier: GPL-2.0+ */

#include <barrier.h>
#include <io.h>
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
#include <drivers/pmu/axp.h>

static axp_pmu_t *dram_pmu_axp2202;

extern int init_DRAM(int type, void *buff);

int set_ddr_voltage(int set_vol)
{
	printk_info("Set DRAM Voltage to %dmv\n", set_vol);
	if (dram_pmu_axp2202 != NULL)
		pmu_axp2202_set_vol(dram_pmu_axp2202, "dcdc3", set_vol, 1);
	return 0;
}

int set_ddr4_2v5_voltage(int set_vol)
{
	printk_info("Set DDR4 25 DRAM Voltage to %dmv\n", set_vol);
	return 0;
}

uint32_t sunxi_dram_init(sunxi_dram_t *dram)
{
	if (dram == NULL || dram->parameter_count == 0U)
		return 0U;
	dram_pmu_axp2202 = dram->pmu;
	dram->size = init_DRAM(0, dram->parameters);
	return dram->size;
}

DT2C_DRIVER_COMPAT("allwinner,sunxi-dram");
