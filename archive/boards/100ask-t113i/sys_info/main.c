/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <common.h>
#include <log.h>
#include <drivers/clk/clk.h>

#include <drivers/dram/dram.h>
#include <drivers/soc/sid.h>
#include <dt-compatible/dram-dt.h>
#include <dt-compatible/sid-dt.h>

#include <jmp.h>

extern sunxi_serial_t uart_dbg;
static sunxi_dram_t dram;

int main(void)
{
	sunxi_sid_t sid;

	sunxi_clk_init();
	if (sunxi_sid_dt_read_alias(&sid, "sid0") != DRIVER_OK) {
		printk_error("SID: invalid devicetree configuration\n");
		return -1;
	}

	if (sunxi_dram_dt_read_alias(&dram, "dram0") != DRIVER_OK) {
		printk_error("DRAM: invalid devicetree configuration\n");
		return -1;
	}
	sunxi_dram_init(&dram);

	printk_info("Hello World!\n");

	sunxi_efuse_dump(&sid);

	sunxi_clk_reset();

	clean_syterkit_data();

	jmp_to_fel();

	return 0;
}
