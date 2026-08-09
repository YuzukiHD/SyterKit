/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <types.h>
#include <stdint.h>

#include <log.h>

#include <common.h>

#include <drivers/pmu/axp.h>
#include <drivers/dram.h>
#include <drivers/i2c.h>

extern sunxi_serial_t uart_dbg;

extern sunxi_i2c_t i2c_pmu;

extern void set_cpu_poweroff(void);

int main(void) {

	show_banner();

	sunxi_clk_init();

	sunxi_clk_dump();

	set_cpu_poweroff();

	neon_enable();

	sunxi_i2c_init(&i2c_pmu);

	pmu_axp1530_init(&i2c_pmu);

	pmu_axp1530_dump(&i2c_pmu);

	int set_vol = 1100; /* LPDDR4 1100mv */

	int temp_vol, src_vol = pmu_axp1530_get_vol(&i2c_pmu, "dcdc3");
	if (src_vol > set_vol) {
		for (temp_vol = src_vol; temp_vol >= set_vol; temp_vol -= 50) { pmu_axp1530_set_vol(&i2c_pmu, "dcdc3", temp_vol, 1); }
	} else if (src_vol < set_vol) {
		for (temp_vol = src_vol; temp_vol <= set_vol; temp_vol += 50) { pmu_axp1530_set_vol(&i2c_pmu, "dcdc3", temp_vol, 1); }
	}

	mdelay(30); /* Delay 300ms for pmu bootup */

	pmu_axp1530_dump(&i2c_pmu);

	printk_info("DRAM: DRAM Size = %dMB\n", sunxi_dram_init(NULL));

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