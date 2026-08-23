/* SPDX-License-Identifier: GPL-2.0+ */

#include <drivers/serial/serial.h>

#include <stdbool.h>
#include <stddef.h>
#include <types.h>
#include <stdint.h>

#include <log.h>
#include <drivers/clk/clk.h>

#include <common.h>

#include <drivers/pmu/axp.h>
#include <drivers/dram/dram.h>
#include <dt-compatible/dram-dt.h>
#include <drivers/i2c/i2c.h>
#include <dt-compatible/i2c-dt.h>

extern sunxi_serial_t uart_dbg;


extern void set_cpu_poweroff(void);

int main(void) {
	sunxi_dram_t dram = {0};
	axp_pmu_t pmu;
	sunxi_i2c_t i2c;

	if (sunxi_serial_init_stdout() != 0)
		return -1;

	show_banner();
	if (sunxi_i2c_dt_read_alias(&i2c, "i2c0") != DRIVER_OK ||
	    pmu_axp1530_config(&pmu, &i2c) != DRIVER_OK) {
		printk_error("PMU: invalid devicetree configuration\n");
		return -1;
	}


	sunxi_clk_init();

	sunxi_clk_dump();

	set_cpu_poweroff();

	neon_enable();

	sunxi_i2c_init(&i2c);

	pmu_axp1530_init(&pmu);

	pmu_axp1530_dump(&pmu);

	int set_vol = 1100; /* LPDDR4 1100mv */

	int temp_vol, src_vol = pmu_axp1530_get_vol(&pmu, "dcdc3");
	if (src_vol > set_vol) {
		for (temp_vol = src_vol; temp_vol >= set_vol; temp_vol -= 50) { pmu_axp1530_set_vol(&pmu, "dcdc3", temp_vol, 1); }
	} else if (src_vol < set_vol) {
		for (temp_vol = src_vol; temp_vol <= set_vol; temp_vol += 50) { pmu_axp1530_set_vol(&pmu, "dcdc3", temp_vol, 1); }
	}

	mdelay(30); /* Delay 300ms for pmu bootup */

	pmu_axp1530_dump(&pmu);

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
