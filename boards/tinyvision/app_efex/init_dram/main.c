/* SPDX-License-Identifier: GPL-2.0+ */

#include <drivers/serial/serial.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>
#include <drivers/clk/clk.h>

#include <common.h>
#include <drivers/dram/dram.h>
#include <dt-compatible/dram-dt.h>

#include <config.h>
#include <efex.h>

extern sunxi_serial_t uart_dbg;
static sunxi_dram_t dram;

int main(void)
{
	if (sunxi_serial_init_stdout() != 0)
		return -1;

	show_banner();

	sunxi_clk_init();

	if (sunxi_dram_dt_read_alias(&dram, "dram0") != DRIVER_OK) {
		pr_err("DRAM: invalid devicetree configuration\n");
		return -1;
	}
	if (sunxi_dram_init(&dram) == 0U) {
		pr_err("DRAM: initialization failed\n");
		return -1;
	}
	syterkit_efex_set_dram_result(dram.parameters, dram.parameter_count);

	return 0;
}
