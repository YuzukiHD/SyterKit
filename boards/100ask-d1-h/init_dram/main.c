/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <types.h>
#include <stdint.h>

#include <log.h>

#include <common.h>

#include <drivers/pmu/axp.h>
#include <drivers/dram.h>
#include <drivers/i2c.h>

extern sunxi_serial_t uart_dbg;

extern dram_para_t dram_para;

int main(void) {

	show_banner();

	sunxi_clk_init();

	sunxi_clk_dump();

	printk_info("DRAM: DRAM Size = %dMB\n", sunxi_dram_init(&dram_para));

	sunxi_clk_dump();

	int i = 0;

	while (1) {
		i++;
		printk_info("Count: %d\n", i);
		mdelay(1000);
	}

	abort();

	return 0;
}