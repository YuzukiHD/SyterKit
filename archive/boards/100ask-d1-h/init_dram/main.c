/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <types.h>
#include <stdint.h>

#include <log.h>
#include <drivers/clk/clk.h>

#include <common.h>

#include <drivers/pmu/axp.h>
#include <drivers/dram/dram.h>
#include <drivers/i2c/i2c.h>
#include <dt-compatible/dram-dt.h>

extern sunxi_serial_t uart_dbg;

static sunxi_dram_t dram;

int main(void)
{
	show_banner();

	sunxi_clk_init();

	sunxi_clk_dump();

	if (sunxi_dram_dt_read_alias(&dram, "dram0") != DRIVER_OK) {
		pr_err("DRAM: invalid devicetree configuration\n");
		return -1;
	}
	pr_info("DRAM: DRAM Size = %dMB\n", sunxi_dram_init(&dram));

	sunxi_clk_dump();

	int i = 0;

	while (1) {
		i++;
		pr_info("Count: %d\n", i);
		mdelay(1000);
	}

	abort();

	return 0;
}
