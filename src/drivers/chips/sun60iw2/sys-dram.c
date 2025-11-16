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

#include <reg-ncat.h>

#include <sys-dram.h>
#include <sys-rtc.h>
#include <pmu/axp.h>

extern sunxi_i2c_t i2c_pmu;

static uint32_t dram_size;

void sunxi_smc_en_with_glitch_workaround(void) {
	return;
}

int set_ddr_voltage_ext(char *name, int set_vol, int on) {
	printk_debug("PMU: %s set vol %d, onoff %d\n", name, set_vol, on);
	pmu_axp8191_set_vol(&i2c_pmu, name, set_vol, on);
	return 0;
}

uint32_t sunxi_get_dram_size() {
	return dram_size;
}

uint32_t sunxi_dram_init(void *para) {
	dram_size = init_DRAM(0, para);
	return dram_size;
}
