/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>
#include <dt-compatible/ccu-dt.h>

#include <common.h>

#include <mmu.h>

#include <drivers/dram.h>
#include <dt-compatible/dram-dt.h>
#include <drivers/gpio.h>
#include <drivers/i2c.h>
#include <drivers/pmu/axp.h>
#include <drivers/sid.h>
#include <drivers/mmc/sdcard.h>
#include <drivers/spi.h>
#include <drivers/serial.h>
#include <dt-compatible/i2c-dt.h>
#include <dt-compatible/mmc-dt.h>
#include <dt-compatible/pmu-dt.h>

extern sunxi_serial_t uart_dbg;


extern void set_cpu_poweroff(void);

void set_pmu_fin_voltage(axp_pmu_t *pmu, char *power_name,
			 uint32_t voltage) {
	int temp_vol, src_vol = pmu_axp1530_get_vol(pmu, power_name);
	if (src_vol > voltage) {
		for (temp_vol = src_vol; temp_vol >= voltage; temp_vol -= 50) { pmu_axp1530_set_vol(pmu, power_name, temp_vol, 1); }
	} else if (src_vol < voltage) {
		for (temp_vol = src_vol; temp_vol <= voltage; temp_vol += 50) { pmu_axp1530_set_vol(pmu, power_name, temp_vol, 1); }
	}
	mdelay(30); /* Delay 300ms for pmu bootup */
}

int main(void) {
	sunxi_ccu_t ccu;
	sunxi_dram_t dram;
	axp_pmu_t pmu;
	sdmmc_pdata_t mmc_card = {0};
	sunxi_i2c_t i2c;
	sunxi_sdhci_t sdhci0;

	show_banner();
	if (sunxi_i2c_dt_read_alias(&i2c, "i2c0") != DRIVER_OK ||
	    sunxi_pmu_dt_read_alias(&pmu, "pmu0", &i2c) != DRIVER_OK) {
		printk_error("PMU: invalid devicetree configuration\n");
		return -1;
	}
	if (sunxi_sdhci_dt_read_alias(&sdhci0, "mmc0") != DRIVER_OK) {
		printk_error("SMHC: invalid devicetree configuration\n");
		return -1;
	}

	if (sunxi_ccu_dt_read(&ccu) != DRIVER_OK) {
		printk_error("CCU: invalid devicetree configuration\n");
		return -1;
	}

	sunxi_clk_init(&ccu);

	sunxi_clk_dump(&ccu);

	set_cpu_poweroff();

	neon_enable();

	sunxi_i2c_init(&i2c);

	pmu_axp1530_init(&pmu);

	pmu_axp1530_dump(&pmu);

	set_pmu_fin_voltage(&pmu, "dcdc2", 1100);
	set_pmu_fin_voltage(&pmu, "dcdc3", 1100);

	pmu_axp1530_dump(&pmu);

	if (sunxi_dram_dt_read_alias(&dram, "dram0", NULL, NULL) != DRIVER_OK) {
		printk_error("DRAM: invalid devicetree configuration\n");
		return -1;
	}
	printk_info("DRAM: DRAM Size = %dMB\n", sunxi_dram_init(&dram));

	sunxi_clk_dump(&ccu);

	/* Initialize the SD host controller. */
	if (sunxi_sdhci_init(&sdhci0) != 0) {
		printk_error("SMHC: %s controller init failed\n", sdhci0.name);
	} else {
		printk_info("SMHC: %s controller initialized\n", sdhci0.name);
	}

	/* Initialize the SD card and check if initialization is successful. */
	if (sdmmc_init(&mmc_card, &sdhci0) != 0) {
		printk_warning("SMHC: init failed\n");
	}

	printk_debug("Card OK!\n");

	abort();

	return 0;
}
