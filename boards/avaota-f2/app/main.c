/* SPDX-License-Identifier: GPL-2.0+ */

#include <drivers/serial/serial.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <types.h>

#include <dt-bindings/soc/sun252iw1.h>
#include <log.h>
#include <drivers/clk/clk.h>

#include <drivers/dma/dma.h>
#include <drivers/dram/dram.h>
#include <dt-compatible/dram-dt.h>
#include <drivers/gpio/gpio.h>
#include <drivers/i2c/i2c.h>
#include <drivers/mmc/sdcard.h>
#include <drivers/mmc/sdhci.h>
#include <drivers/mtd/spi-nand.h>
#include <drivers/mtd/spi-nor.h>
#include <drivers/pmu/axp.h>
#include <drivers/spi/spi.h>
#include <dt-compatible/i2c-dt.h>

#include <common.h>

#include <cli/cli.h>
#include <cli/cli_shell.h>
#include <cli/cli_termesc.h>

#include <e907/sysmap.h>


void sunxi_pmc_config(void) {
	if (!(readl(SUNXI_RTC_PMC_BYPASS_STATUS) & BIT(0))) {
		/* if PMC bypass, restore all IO to GPIO */
		writel(BIT(0) | BIT(1) | BIT(2) | BIT(5), SUNXI_RTC_IOMODE_CTL);
	}
}

int main(void) {
	sunxi_dram_t dram = {0};
	axp_pmu_t pmu;
	sunxi_i2c_t i2c;

	if (sunxi_serial_init_stdout() != 0)
		return -1;

	show_banner();
	if (sunxi_i2c_dt_read_alias(&i2c, "i2c0") != DRIVER_OK ||
	    pmu_axp333_config(&pmu, &i2c) != DRIVER_OK) {
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

	if (sunxi_dram_dt_read_alias(&dram, "dram0") != DRIVER_OK) {
		printk_error("DRAM: invalid devicetree configuration\n");
		return -1;
	}
	sunxi_dram_init(&dram);

	syterkit_shell_attach(NULL);

	abort();

	return 0;
}
