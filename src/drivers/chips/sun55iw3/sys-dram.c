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

extern int init_DRAM(int type, void *buff);

static uint32_t dram_size;

int set_ddr_voltage(unsigned int vol_val) {
	/* ddr voltage already set in main */
	printk_info("set_ddr_voltage: %d\n", vol_val);
	return 0;
}

uint32_t sunxi_get_dram_size() {
	return dram_size;
}

uint32_t sunxi_dram_init(void *para) {
	dram_size = init_DRAM(0, para);
	return dram_size;
}
