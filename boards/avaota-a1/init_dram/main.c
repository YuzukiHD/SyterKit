/* SPDX-License-Identifier: GPL-2.0+ */

#include <drivers/serial/serial.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>
#include <drivers/clk/clk.h>
#include <mmu.h>

#include <common.h>

#include <drivers/pmu/axp.h>
#include <dt-compatible/i2c-dt.h>
#include <drivers/clk/sun55iw3/clk.h>
#include <drivers/clk/sun55iw3/reg.h>
#include <drivers/dram/dram.h>
#include <dt-compatible/dram-dt.h>
#include <drivers/i2c/i2c.h>
#include <drivers/remoteproc/remoteproc.h>
#include <dt-compatible/remoteproc-dt.h>

extern sunxi_serial_t uart_dbg;

extern void set_rpio_power_mode(void);

#include "memtester.c"

static sunxi_dram_t dram;

int main(void)
{
	axp_pmu_t axp2202;
	axp_pmu_t axp1530;
	sunxi_i2c_t i2c;
	sunxi_remoteproc_t e906;

	arm32_dcache_enable();
	arm32_icache_enable();

	if (sunxi_serial_init_stdout() != 0)
		return -1;

	show_banner();
	if (sunxi_remoteproc_dt_read_alias(&e906, "e906", NULL) != DRIVER_OK) {
		pr_err("RISC-V E906: invalid devicetree configuration\n");
		return -1;
	}
	if (sunxi_i2c_dt_read_alias(&i2c, "i2c0") != DRIVER_OK || pmu_axp2202_config(&axp2202, &i2c) != DRIVER_OK || pmu_axp1530_config(&axp1530, &i2c) != DRIVER_OK) {
		pr_err("PMU: invalid devicetree configuration\n");
		return -1;
	}

	sunxi_clk_init();

	set_rpio_power_mode();

	sunxi_clk_dump();

	sunxi_i2c_init(&i2c);

	pmu_axp2202_init(&axp2202);

	pmu_axp1530_init(&axp1530);

	pmu_axp2202_set_vol(&axp2202, "dcdc1", 1100, 1);

	pmu_axp1530_set_dual_phase(&axp1530);
	pmu_axp1530_set_vol(&axp1530, "dcdc1", 1100, 1);
	pmu_axp1530_set_vol(&axp1530, "dcdc2", 1100, 1);

	pmu_axp2202_set_vol(&axp2202, "dcdc2", 920, 1);
	pmu_axp2202_set_vol(&axp2202, "dcdc3", 1160, 1);
	pmu_axp2202_set_vol(&axp2202, "dcdc4", 3300, 1);

	pmu_axp2202_set_vol(&axp2202, "bldo3", 1800, 1);
	pmu_axp2202_set_vol(&axp2202, "bldo1", 1800, 1);

	pmu_axp2202_dump(&axp2202);
	pmu_axp1530_dump(&axp1530);

	sun55iw3_clk_set_cpu_pll(1800);

	if (sunxi_remoteproc_reset(&e906) != DRIVER_OK) {
		pr_err("RISC-V E906: reset failed\n");
		return -1;
	}

	if (sunxi_dram_dt_read_alias(&dram, "dram0") != DRIVER_OK) {
		pr_err("DRAM: invalid devicetree configuration\n");
		return -1;
	}
	uint32_t dram_size = sunxi_dram_init(&dram);
	arm32_mmu_enable(dram.memory_base, dram_size);
	pr_info("DRAM: DRAM Size = %dMB", dram_size);

	/* PLL DDR0 */
	uint32_t reg32 = read32(SUNXI_CCMU_BASE + CCU_PLL_DDR0_CTRL_REG);
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
		do_memtester((uint64_t)dram.memory_base, DRAM_SIZE_BYTE, DRAM_TEST_SIZE, i);
		do_memtester((uint64_t)dram.memory_base + (uint64_t)0x40000000, DRAM_SIZE_BYTE, DRAM_TEST_SIZE, i);
		do_memtester((uint64_t)dram.memory_base + (uint64_t)0x80000000, DRAM_SIZE_BYTE, DRAM_TEST_SIZE, i);
		i++;
	}

	abort();

	return 0;
}
