/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <types.h>

#include <log.h>

#include <drivers/clk.h>
#include <drivers/dma.h>
#include <drivers/dram.h>
#include <drivers/gpio.h>
#include <drivers/i2c.h>
#include <drivers/sdcard.h>
#include <drivers/sdhci.h>
#include <drivers/mtd/spi-nand.h>
#include <drivers/mtd/spi-nor.h>
#include <drivers/pmu/axp.h>
#include <drivers/spi.h>
#include <dt-compatible/i2c-dt.h>
#include <dt-compatible/pmu-dt.h>

#include <common.h>

#include <cli/cli.h>
#include <cli/cli_shell.h>
#include <cli/cli_termesc.h>

#include <e907/sysmap.h>

extern sunxi_serial_t uart_dbg_ph1;
extern sunxi_serial_t uart_dbg;
extern dram_para_t dram_para;

void sunxi_pmc_config(void) {
	if (!(readl(SUNXI_RTC_PMC_BYPASS_STATUS) & BIT(0))) {
		/* if PMC bypass, restore all IO to GPIO */
		writel(BIT(0) | BIT(1) | BIT(2) | BIT(5), SUNXI_RTC_IOMODE_CTL);
	}
}

int main(void) {
	axp_pmu_t pmu;
	sunxi_i2c_t i2c;

	show_banner();
	if (sunxi_i2c_dt_read_alias(&i2c, "i2c0") != DRIVER_OK ||
	    sunxi_pmu_dt_read_alias(&pmu, "pmu0", &i2c) != DRIVER_OK) {
		printk_error("PMU: invalid devicetree configuration\n");
		return -1;
	}

	sysmap_dump_region_info();

	sunxi_clk_dump();

	sunxi_clk_init();

	printk_info("Hello World!\n");

	sunxi_clk_dump();

	sunxi_pmc_config();

	sunxi_i2c_init(&i2c);

	pmu_axp333_init(&pmu);

	pmu_axp333_set_vol(&pmu, "dcdc2", 1500, 1);

	pmu_axp333_dump(&pmu);

	sunxi_dram_init((void *) &dram_para);

	syterkit_shell_attach(NULL);

	abort();

	return 0;
}
