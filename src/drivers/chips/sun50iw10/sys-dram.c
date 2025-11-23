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

#include <sys-dram.h>
#include <sys-rtc.h>
#include <pmu/axp.h>

static uint32_t dram_size;

extern sunxi_i2c_t i2c_pmu;

extern int init_DRAM(int type, void *buff);

int set_ddr_voltage(int set_vol) {
	printk_info("Set DRAM Voltage to %dmv\n", set_vol);
	pmu_axp2202_set_vol(&i2c_pmu, "dcdc3", set_vol, 1);
	return 0;
}

int set_ddr4_2v5_voltage(int set_vol) {
	printk_info("Set DDR4 25 DRAM Voltage to %dmv\n", set_vol);
	return 0;
}

uint32_t sunxi_get_dram_size() {
	return dram_size;
}

uint32_t sunxi_dram_init(void *para) {
	dram_size = init_DRAM(0, para);
	return dram_size;
}