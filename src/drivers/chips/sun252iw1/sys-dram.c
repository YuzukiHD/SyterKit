/* SPDX-License-Identifier: GPL-2.0+ */

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>
#include <mmu.h>

#include <sys-clk.h>
#include <sys-dram.h>
#include <reg-dram.h>
#include <pmu/axp.h>

#include <common.h>

extern int init_DRAM(int type, void *buff);

static uint32_t dram_size;

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

uint32_t sunxi_get_dram_size() {
	return dram_size;
}

uint32_t sunxi_dram_init(void *para) {
	dram_size = init_DRAM(0, para);
	return dram_size;
}