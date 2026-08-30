/* SPDX-License-Identifier: GPL-2.0+ */

#include <common.h>
#include <log.h>
#include <drivers/clk/clk.h>
#include <drivers/dram/dram.h>
#include <drivers/i2c/i2c.h>
#include <drivers/pmu/axp.h>
#include <drivers/serial/serial.h>
#include <dt-compatible/dram-dt.h>
#include <dt-compatible/i2c-dt.h>
#include <efex.h>

extern void board_common_init(void);

int main(void)
{
	sunxi_dram_t dram = { 0 };
	axp_pmu_t pmu;
	sunxi_i2c_t i2c;

	if (sunxi_serial_init_stdout() != 0)
		return -1;
	if (sunxi_i2c_dt_read_alias(&i2c, "i2c0") != DRIVER_OK ||
	    pmu_axp8191_config(&pmu, &i2c) != DRIVER_OK)
		return -1;
	board_common_init();
	sunxi_i2c_init(&i2c);
	sunxi_clk_init();
	pmu_axp8191_init(&pmu);
	dram.power.ddr = &pmu;
	if (sunxi_dram_dt_read_alias(&dram, "dram0") != DRIVER_OK ||
	    sunxi_dram_init(&dram) == 0U) {
		pr_err("DRAM: initialization failed\n");
		return -1;
	}
	syterkit_efex_set_dram_result(dram.parameters, dram.parameter_count);
	return 0;
}
