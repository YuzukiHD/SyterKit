/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>

#include <common.h>

#include <mmu.h>

#include <cli.h>
#include <cli_shell.h>
#include <cli_termesc.h>

#include <sys-dram.h>
#include <sys-gpio.h>
#include <sys-i2c.h>
#include <sys-sdhci.h>
#include <sys-sdcard.h>
#include <sys-sid.h>
#include <sys-spi.h>
#include <sys-uart.h>

#define CONFIG_SDMMC_SPEED_TEST_SIZE 102400 * 4

extern sunxi_serial_t uart_dbg;

extern sunxi_i2c_t i2c_pmu;

extern sunxi_sdhci_t sdhci2;

extern uint32_t dram_para[32];

msh_declare_command(speedtest);
msh_define_help(speedtest, "Do speed test", "Usage: speedtest\n");
int cmd_speedtest(int argc, const char **argv) {
	uint32_t start;
	uint32_t test_time;

	start = time_ms();
	sdmmc_blk_write(&card0, (uint8_t *) (SDRAM_BASE), 0, CONFIG_SDMMC_SPEED_TEST_SIZE);
	test_time = time_ms() - start;
	printk_info("SDMMC: Write speedtest %uKB in %ums at %uKB/S\n", (CONFIG_SDMMC_SPEED_TEST_SIZE * 512) / 1024, test_time, (CONFIG_SDMMC_SPEED_TEST_SIZE * 512) / test_time);

	start = time_ms();
	sdmmc_blk_read(&card0, (uint8_t *) (SDRAM_BASE), 0, CONFIG_SDMMC_SPEED_TEST_SIZE);
	test_time = time_ms() - start;
	printk_info("SDMMC: Read speedtest %uKB in %ums at %uKB/S\n", (CONFIG_SDMMC_SPEED_TEST_SIZE * 512) / 1024, test_time, (CONFIG_SDMMC_SPEED_TEST_SIZE * 512) / test_time);

	return 0;
}

msh_declare_command(swi);
msh_define_help(swi, "Software interrupt test", "Usage: swi\n");
int cmd_swi(int argc, const char **argv) {
	asm volatile("svc #0");
	return 0;
}

const msh_command_entry commands[] = {
		msh_define_command(speedtest),
		msh_define_command(swi),
		msh_command_end,
};

int main(void) {
	sunxi_serial_init(&uart_dbg);

	arm32_dcache_enable();
	arm32_icache_enable();

	show_banner();

	rtc_set_vccio_det_spare();

	sunxi_clk_init();

	set_rpio_power_mode();

	sunxi_clk_dump();

	sunxi_i2c_init(&i2c_pmu);

	pmu_axp2202_init(&i2c_pmu);

	pmu_axp1530_init(&i2c_pmu);

	pmu_axp2202_set_vol(&i2c_pmu, "dcdc1", 1100, 1);

	pmu_axp1530_set_dual_phase(&i2c_pmu);
	pmu_axp1530_set_vol(&i2c_pmu, "dcdc1", 1100, 1);
	pmu_axp1530_set_vol(&i2c_pmu, "dcdc2", 1100, 1);

	pmu_axp2202_set_vol(&i2c_pmu, "dcdc2", 920, 1);
	pmu_axp2202_set_vol(&i2c_pmu, "dcdc3", 1160, 1);
	pmu_axp2202_set_vol(&i2c_pmu, "dcdc4", 3300, 1);

	pmu_axp2202_set_vol(&i2c_pmu, "bldo3", 1800, 1);
	pmu_axp2202_set_vol(&i2c_pmu, "bldo1", 1800, 1);
	pmu_axp2202_set_vol(&i2c_pmu, "cldo1", 2100, 1);

	pmu_axp2202_dump(&i2c_pmu);
	pmu_axp1530_dump(&i2c_pmu);

	sunxi_clk_set_cpu_pll(1416);

	enable_sram_a3();

	/* Initialize the DRAM and enable memory management unit (MMU). */
	uint32_t dram_size = sunxi_dram_init((void *) dram_para);

	printk_debug("DRAM Size = %dM\n", dram_size);

	sunxi_clk_dump();

	arm32_mmu_enable(SDRAM_BASE, dram_size);

	sunxi_clk_dump();

	/* Initialize the SD host controller. */
	if (sunxi_sdhci_init(&sdhci2) != 0) {
		printk_error("SMHC: %s controller init failed\n", sdhci2.name);
	} else {
		printk_info("SMHC: %s controller initialized\n", sdhci2.name);
	}

	/* Initialize the SD card and check if initialization is successful. */
	if (sdmmc_init(&card0, &sdhci2) != 0) {
		printk_warning("SMHC: init failed\n");
	} else {
		printk_debug("Card OK!\n");
	}

	syterkit_shell_attach(commands);

	return 0;
}