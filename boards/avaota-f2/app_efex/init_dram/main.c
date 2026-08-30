/* SPDX-License-Identifier: GPL-2.0+ */

#include <common.h>
#include <io.h>
#include <log.h>
#include <dt-bindings/soc/sun252iw1.h>
#include <drivers/clk/clk.h>
#include <drivers/dram/dram.h>
#include <drivers/i2c/i2c.h>
#include <drivers/pmu/axp.h>
#include <drivers/serial/serial.h>
#include <dt-compatible/dram-dt.h>
#include <dt-compatible/i2c-dt.h>
#include <efex.h>

static void sunxi_pmc_config(void)
{
	if (!(readl(SUNXI_RTC_PMC_BYPASS_STATUS) & BIT(0)))
		writel(BIT(0) | BIT(1) | BIT(2) | BIT(5), SUNXI_RTC_IOMODE_CTL);
}

int main(void)
{
	sunxi_dram_t dram = { 0 };
	axp_pmu_t pmu;
	sunxi_i2c_t i2c;

	if (sunxi_serial_init_stdout() != 0)
		return -1;
	if (sunxi_i2c_dt_read_alias(&i2c, "i2c2") != DRIVER_OK ||
	    pmu_axp333_config(&pmu, &i2c) != DRIVER_OK) {
		pr_err("PMU: invalid devicetree configuration\n");
		return -1;
	}
	sunxi_clk_init();
	sunxi_pmc_config();
	sunxi_i2c_init(&i2c);
	pmu_axp333_init(&pmu);
	pmu_axp333_set_vol(&pmu, "dcdc2", 1500, 1);
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
