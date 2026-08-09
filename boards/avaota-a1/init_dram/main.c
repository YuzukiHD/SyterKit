/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>
#include <mmu.h>

#include <common.h>

#include <drivers/pmu/axp.h>
#include <dt-compatible/i2c-dt.h>
#include <dt-compatible/pmu-dt.h>
#include <drivers/clk.h>
#include <drivers/dram.h>
#include <drivers/i2c.h>

extern sunxi_serial_t uart_dbg;
extern uint32_t dram_para[32];

extern void set_rpio_power_mode(void);
extern void rtc_set_vccio_det_spare(void);

#include "memtester.c"


int main(void) {
	axp_pmu_t primary_pmu;
	axp_pmu_t secondary_pmu;
	sunxi_i2c_t i2c;

	arm32_dcache_enable();
	arm32_icache_enable();

	show_banner();
	if (sunxi_i2c_dt_read_alias(&i2c, "i2c0") != DRIVER_OK ||
	    sunxi_pmu_dt_read_alias(&primary_pmu, "pmu0", &i2c) != DRIVER_OK ||
	    sunxi_pmu_dt_read_alias(&secondary_pmu, "pmu1", &i2c) != DRIVER_OK) {
		printk_error("PMU: invalid devicetree configuration\n");
		return -1;
	}

	rtc_set_vccio_det_spare();

	sunxi_clk_init();

	set_rpio_power_mode();

	sunxi_clk_dump();

	sunxi_i2c_init(&i2c);

	pmu_axp2202_init(&primary_pmu);

	pmu_axp1530_init(&secondary_pmu);

	pmu_axp2202_set_vol(&primary_pmu, "dcdc1", 1100, 1);

	pmu_axp1530_set_dual_phase(&secondary_pmu);
	pmu_axp1530_set_vol(&secondary_pmu, "dcdc1", 1100, 1);
	pmu_axp1530_set_vol(&secondary_pmu, "dcdc2", 1100, 1);

	pmu_axp2202_set_vol(&primary_pmu, "dcdc2", 920, 1);
	pmu_axp2202_set_vol(&primary_pmu, "dcdc3", 1160, 1);
	pmu_axp2202_set_vol(&primary_pmu, "dcdc4", 3300, 1);

	pmu_axp2202_set_vol(&primary_pmu, "bldo3", 1800, 1);
	pmu_axp2202_set_vol(&primary_pmu, "bldo1", 1800, 1);

	pmu_axp2202_dump(&primary_pmu);
	pmu_axp1530_dump(&secondary_pmu);

	sunxi_clk_set_cpu_pll(1800);

	enable_sram_a3();

	uint32_t dram_size = sunxi_dram_init(dram_para);
	arm32_mmu_enable(SDRAM_BASE, dram_size);
	printk_info("DRAM: DRAM Size = %dMB", dram_size);

	/* PLL DDR0 */
	uint32_t reg32 = read32(CCU_BASE + CCU_PLL_DDR0_CTRL_REG);
	if (reg32 & (1 << 31)) {
		uint32_t plln = ((reg32 >> 8) & 0xff) + 1;

		uint32_t p1 = ((reg32 >> 1) & 0x1) + 1;
		uint32_t p0 = (reg32 & 0x01) + 1;

		printk(LOG_LEVEL_MUTE, ", DRAM CLK = %luMHz", (24 * plln) / (p0 * p1));
	}

	printk(LOG_LEVEL_MUTE, "\n");

	sunxi_clk_dump();

#define DRAM_TEST_SIZE 32 * 1024 * 1024
#define DRAM_SIZE_BYTE dram_size * 1024 * 1024

	static int i = 0;
	while (1) {
		do_memtester((uint64_t) SDRAM_BASE, DRAM_SIZE_BYTE, DRAM_TEST_SIZE, i);
		do_memtester((uint64_t) SDRAM_BASE + (uint64_t) 0x40000000, DRAM_SIZE_BYTE, DRAM_TEST_SIZE, i);
		do_memtester((uint64_t) SDRAM_BASE + (uint64_t) 0x80000000, DRAM_SIZE_BYTE, DRAM_TEST_SIZE, i);
		i++;
	}

	abort();

	return 0;
}
