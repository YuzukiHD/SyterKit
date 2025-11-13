/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <types.h>

#include <log.h>

#include <sys-clk.h>
#include <sys-dma.h>
#include <sys-dram.h>
#include <sys-gpio.h>
#include <sys-i2c.h>
#include <sys-sdcard.h>
#include <sys-sdhci.h>
#include <sys-spi-nand.h>
#include <sys-spi-nor.h>
#include <sys-spi.h>

#include <common.h>

#include <cli.h>
#include <cli_shell.h>
#include <cli_termesc.h>

#include <e907/sysmap.h>

extern sunxi_serial_t uart_dbg_ph1;
extern sunxi_serial_t uart_dbg;
extern sunxi_i2c_t i2c_pmu;
extern dram_para_t dram_para;

void sunxi_pmc_config(void) {
	if (!(readl(SUNXI_RTC_PMC_BYPASS_STATUS) & BIT(0))) {
		/* if PMC bypass, restore all IO to GPIO */
		writel(BIT(0) | BIT(1) | BIT(2) | BIT(5), SUNXI_RTC_IOMODE_CTL);
	}
}

int main(void) {
	uint32_t start, time;

	sunxi_serial_init(&uart_dbg);

	show_banner();

	sysmap_dump_region_info();

	sunxi_clk_dump();

	sunxi_clk_init();

	printk_info("Hello World!\n");

	sunxi_clk_dump();

	sunxi_pmc_config();

	sunxi_i2c_init(&i2c_pmu);

	pmu_axp333_init(&i2c_pmu);

	pmu_axp333_set_vol(&i2c_pmu, "dcdc2", 1500, 1);

	pmu_axp333_dump(&i2c_pmu);

	sunxi_dram_init((void *) &dram_para);

	syterkit_shell_attach(NULL);

	abort();

	return 0;
}