/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <common.h>
#include <log.h>
#include <dt-compatible/ccu-dt.h>

#include <drivers/dram.h>
#include <drivers/sid.h>
#include <dt-compatible/dram-dt.h>
#include <dt-compatible/sid-dt.h>

#include <jmp.h>

extern sunxi_serial_t uart_dbg;
static sunxi_dram_t dram;

int main(void) {
	sunxi_ccu_t ccu;
	sunxi_sid_t sid;

	if (sunxi_ccu_dt_read(&ccu) != DRIVER_OK) {
		printk_error("CCU: invalid devicetree configuration\n");
		return -1;
	}

	sunxi_clk_init(&ccu);
	if (sunxi_sid_dt_read_alias(&sid, "sid0") != DRIVER_OK) {
		printk_error("SID: invalid devicetree configuration\n");
		return -1;
	}

	if (sunxi_dram_dt_read_alias(&dram, "dram0", NULL, NULL) != DRIVER_OK) {
		printk_error("DRAM: invalid devicetree configuration\n");
		return -1;
	}
	sunxi_dram_init(&dram);

	printk_info("Hello World!\n");

	syter_efuse_dump(&sid);

	sunxi_clk_reset(&ccu);

	clean_syterkit_data();

	jmp_to_fel();

	return 0;
}
