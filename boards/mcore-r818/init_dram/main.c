/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>

#include <common.h>

#include <drivers/pmu/axp.h>
#include <drivers/dram.h>
#include <drivers/i2c.h>
#include <dt-compatible/i2c-dt.h>
#include <dt-compatible/pmu-dt.h>

extern sunxi_serial_t uart_dbg;
extern uint32_t dram_para[32];

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
	axp_pmu_t pmu;
	sunxi_i2c_t i2c;

	show_banner();
	if (sunxi_i2c_dt_read_alias(&i2c, "i2c0") != DRIVER_OK ||
	    sunxi_pmu_dt_read_alias(&pmu, "pmu0", &i2c) != DRIVER_OK) {
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

	printk_info("DRAM: DRAM Size = %dMB\n",
		    sunxi_dram_init_with_pmu(&dram_para, &pmu, NULL));

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
