/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>

#include <common.h>

#include <pmu/axp.h>
#include <sys-dram.h>
#include <sys-i2c.h>

extern sunxi_serial_t uart_dbg;
extern uint32_t dram_para[32];
extern sunxi_i2c_t i2c_pmu;

static void set_pmu_fin_voltage(char *power_name, uint32_t voltage) {
	int set_vol = voltage;
	int temp_vol, src_vol = pmu_axp2202_get_vol(&i2c_pmu, power_name);
	if (src_vol > voltage) {
		for (temp_vol = src_vol; temp_vol >= voltage; temp_vol -= 50) { pmu_axp2202_set_vol(&i2c_pmu, power_name, temp_vol, 1); }
	} else if (src_vol < voltage) {
		for (temp_vol = src_vol; temp_vol <= voltage; temp_vol += 50) { pmu_axp2202_set_vol(&i2c_pmu, power_name, temp_vol, 1); }
	}
	mdelay(30); /* Delay 300ms for pmu bootup */
}

int main(void) {
	sunxi_serial_init(&uart_dbg);

	show_banner();

	sunxi_clk_init();

	sunxi_clk_dump();

	sunxi_i2c_init(&i2c_pmu);

	pmu_axp2202_init(&i2c_pmu);

	set_pmu_fin_voltage("dcdc1", 1100);
	set_pmu_fin_voltage("dcdc3", 1160);

	mdelay(30); /* Delay 300ms for pmu bootup */

	pmu_axp2202_dump(&i2c_pmu);

	printk_info("DRAM: DRAM Size = %dMB\n", sunxi_dram_init(&dram_para));

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