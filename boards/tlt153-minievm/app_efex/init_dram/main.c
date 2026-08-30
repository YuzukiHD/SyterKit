/* SPDX-License-Identifier: GPL-2.0+ */

#include <common.h>
#include <log.h>
#include <drivers/clk/clk.h>
#include <drivers/dram/dram.h>
#include <drivers/serial/serial.h>
#include <dt-compatible/dram-dt.h>
#include <efex.h>

int main(void)
{
	sunxi_dram_t dram = { 0 };

	if (sunxi_serial_init_stdout() != 0)
		return -1;
	sunxi_clk_init();
	if (sunxi_dram_dt_read_alias(&dram, "dram0") != DRIVER_OK ||
	    sunxi_dram_init(&dram) == 0U) {
		pr_err("DRAM: initialization failed\n");
		return -1;
	}
	syterkit_efex_set_dram_result(dram.parameters, dram.parameter_count);
	return 0;
}
