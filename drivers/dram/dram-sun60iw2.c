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

#include <drivers/reg/reg-ncat.h>

#include <drivers/dram.h>
#include <drivers/rtc.h>
#include <drivers/pmu/axp.h>


static uint32_t dram_size;
static axp_pmu_t *dram_pmu;

extern int init_DRAM(int type, void *buff);

void sunxi_smc_en_with_glitch_workaround(void) {
	return;
}

int set_ddr_voltage_ext(char *name, int set_vol, int on) {
	printk_debug("PMU: %s set vol %d, onoff %d\n", name, set_vol, on);
	pmu_axp8191_set_vol(dram_pmu, name, set_vol, on);
	return 0;
}

uint32_t sunxi_get_dram_size() {
	return dram_size;
}

uint32_t sunxi_dram_init(void *para) {
	dram_size = init_DRAM(0, para);
	return dram_size;
}

uint32_t sunxi_dram_init_with_pmu(void *para, axp_pmu_t *primary,
				  axp_pmu_t *secondary) {
	(void) secondary;
	dram_pmu = primary;
	return sunxi_dram_init(para);
}
