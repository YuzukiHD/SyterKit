/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>
#include <drivers/clk/clk.h>

#include <common.h>

#include <drivers/pmu/axp.h>
#include <drivers/dram/dram.h>
#include <dt-compatible/dram-dt.h>
#include <drivers/i2c/i2c.h>
#include <dt-compatible/i2c-dt.h>

extern sunxi_serial_t uart_dbg;

static void set_pmu_fin_voltage(axp_pmu_t *pmu, char *power_name,
				uint32_t voltage) {
	int temp_vol, src_vol = pmu_axp2202_get_vol(pmu, power_name);
	if (src_vol > voltage) {
		for (temp_vol = src_vol; temp_vol >= voltage; temp_vol -= 50) { pmu_axp2202_set_vol(pmu, power_name, temp_vol, 1); }
	} else if (src_vol < voltage) {
		for (temp_vol = src_vol; temp_vol <= voltage; temp_vol += 50) { pmu_axp2202_set_vol(pmu, power_name, temp_vol, 1); }
	}
	mdelay(30); /* Delay 300ms for pmu bootup */
}

int main(void) {
	sunxi_dram_t dram = {0};
	axp_pmu_t pmu;
	sunxi_i2c_t i2c;

	show_banner();
	if (sunxi_i2c_dt_read_alias(&i2c, "i2c0") != DRIVER_OK ||
	    pmu_axp2202_config(&pmu, &i2c) != DRIVER_OK) {
		printk_error("PMU: invalid devicetree configuration\n");
		return -1;
	}


	sunxi_clk_init();

	sunxi_clk_dump();

	sunxi_i2c_init(&i2c);

	pmu_axp2202_init(&pmu);

	set_pmu_fin_voltage(&pmu, "dcdc1", 1100);
	set_pmu_fin_voltage(&pmu, "dcdc3", 1160);

	mdelay(30); /* Delay 300ms for pmu bootup */

	pmu_axp2202_dump(&pmu);

	dram.pmu = &pmu;
	if (sunxi_dram_dt_read_alias(&dram, "dram0") != DRIVER_OK) {
		printk_error("DRAM: invalid devicetree configuration\n");
		return -1;
	}
	printk_info("DRAM: DRAM Size = %dMB\n", sunxi_dram_init(&dram));

	sunxi_clk_dump();

	int i = 0;

	while (1) {
		i++;
		printk_info("Count: %d\n", i);
		mdelay(1000);
	}

	abort();

	return 0;
}
