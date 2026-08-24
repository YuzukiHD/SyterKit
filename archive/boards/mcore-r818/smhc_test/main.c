/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>
#include <drivers/clk/clk.h>

#include <common.h>

#include <mmu.h>

#include <drivers/dram/dram.h>
#include <dt-compatible/dram-dt.h>
#include <drivers/gpio/gpio.h>
#include <drivers/i2c/i2c.h>
#include <drivers/pmu/axp.h>
#include <drivers/soc/sid.h>
#include <drivers/mmc/sdhci.h>
#include <drivers/mmc/sdcard.h>
#include <drivers/spi/spi.h>
#include <drivers/serial/serial.h>
#include <dt-compatible/i2c-dt.h>
#include <dt-compatible/mmc-dt.h>

extern sunxi_serial_t uart_dbg;

static void set_pmu_fin_voltage(axp_pmu_t *pmu, char *power_name, uint32_t voltage)
{
	int temp_vol, src_vol = pmu_axp2202_get_vol(pmu, power_name);
	if (src_vol > voltage) {
		for (temp_vol = src_vol; temp_vol >= voltage; temp_vol -= 50) {
			pmu_axp2202_set_vol(pmu, power_name, temp_vol, 1);
		}
	} else if (src_vol < voltage) {
		for (temp_vol = src_vol; temp_vol <= voltage; temp_vol += 50) {
			pmu_axp2202_set_vol(pmu, power_name, temp_vol, 1);
		}
	}
	mdelay(30); /* Delay 300ms for pmu bootup */
}

int main(void)
{
	sunxi_dram_t dram = { 0 };
	axp_pmu_t pmu;
	sdmmc_pdata_t mmc_card = { 0 };
	sunxi_i2c_t i2c;
	sunxi_sdhci_t sdhci2 = { 0 };

	show_banner();
	if (sunxi_i2c_dt_read_alias(&i2c, "i2c0") != DRIVER_OK || pmu_axp2202_config(&pmu, &i2c) != DRIVER_OK) {
		printk_error("PMU: invalid devicetree configuration\n");
		return -1;
	}
	if (sunxi_sdhci_dt_read_alias(&sdhci2, "mmc2") != DRIVER_OK) {
		printk_error("SMHC: invalid devicetree configuration\n");
		return -1;
	}

	sunxi_clk_init();

	sunxi_clk_dump();

	sunxi_i2c_init(&i2c);

	pmu_axp2202_init(&pmu);

	set_pmu_fin_voltage(&pmu, "dcdc1", 1100);
	set_pmu_fin_voltage(&pmu, "dcdc3", 1100);

	pmu_axp2202_dump(&pmu);

	dram.pmu = &pmu;
	if (sunxi_dram_dt_read_alias(&dram, "dram0") != DRIVER_OK) {
		printk_error("DRAM: invalid devicetree configuration\n");
		return -1;
	}
	printk_info("DRAM: DRAM Size = %dMB\n", sunxi_dram_init(&dram));

	sunxi_clk_dump();

	/* Initialize the SD host controller. */
	if (sunxi_sdhci_init(&sdhci2) != 0) {
		printk_error("SMHC: %s controller init failed\n", sdhci2.name);
	} else {
		printk_info("SMHC: %s controller initialized\n", sdhci2.name);
	}

	/* Initialize the SD card and check if initialization is successful. */
	if (sdmmc_init(&mmc_card, &sdhci2) != 0) {
		printk_warning("SMHC: init failed\n");
	}

	printk_debug("Card OK!\n");

	abort();

	return 0;
}
