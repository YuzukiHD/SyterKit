/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>
#include <dt-compatible/ccu-dt.h>

#include <common.h>
#include <drivers/dram.h>
#include <dt-compatible/dram-dt.h>

#include <config.h>

extern sunxi_serial_t uart_dbg;
static sunxi_dram_t dram;


int main(void) {
	sunxi_ccu_t ccu;

	show_banner();

	if (sunxi_ccu_dt_read(&ccu) != DRIVER_OK) {
		printk_error("CCU: invalid devicetree configuration\n");
		return -1;
	}

	sunxi_clk_init(&ccu);

	if (sunxi_dram_dt_read_alias(&dram, "dram0", NULL, NULL) != DRIVER_OK) {
		printk_error("DRAM: invalid devicetree configuration\n");
		return -1;
	}
	sunxi_dram_init(&dram);

	return 0;
}
