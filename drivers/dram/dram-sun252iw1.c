/* SPDX-License-Identifier: GPL-2.0+ */

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>
#include <mmu.h>

#include <dt2c/driver.h>
#include <drivers/clk/clk.h>
#include <drivers/dram/dram.h>
#include <drivers/pmu/axp.h>

#include <common.h>

extern int init_DRAM(int type, void *buff);

void __usdelay(unsigned long us) {
	udelay(us);
}

void csi_l2c_clear_invalid_all(void) {
	invalidate_dcache_all();
	return;
}

void csi_l2c_clear_all(void) {
	flush_dcache_all();
	return;
}

int set_ddr_voltage(unsigned int vol_val) {
	return 0;
}

uint32_t sunxi_dram_init(sunxi_dram_t *dram) {
	if (dram == NULL || dram->parameter_count == 0U)
		return 0U;
	dram->size = init_DRAM(0, dram->parameters);
	return dram->size;
}

DT2C_DRIVER_COMPAT("allwinner,sun252iw1-dram");
