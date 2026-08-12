/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>
#include <dt-compatible/ccu-dt.h>

#include <common.h>

#include <mmu.h>

#include <cli/cli.h>
#include <cli/cli_shell.h>
#include <cli/cli_termesc.h>

#include <drivers/dram/dram.h>
#include <drivers/clk/sun55iw3/clk.h>
#include <dt-compatible/dram-dt.h>
#include <drivers/gpio/gpio.h>
#include <drivers/i2c/i2c.h>
#include <drivers/remoteproc/remoteproc.h>
#include <drivers/pmu/axp.h>
#include <dt-compatible/i2c-dt.h>
#include <dt-compatible/mmc-dt.h>
#include <dt-compatible/pmu-dt.h>
#include <dt-compatible/remoteproc-dt.h>
#include <drivers/mmc/sdhci.h>
#include <drivers/mmc/sdcard.h>
#include <drivers/soc/sid.h>
#include <drivers/spi/spi.h>
#include <drivers/serial/serial.h>

static sunxi_dram_t dram;

#define CONFIG_SDMMC_SPEED_TEST_SIZE 102400 * 4

extern sunxi_serial_t uart_dbg;



static sdmmc_pdata_t test_card;
static sunxi_sdhci_t test_mmc;

msh_declare_command(speedtest);
msh_define_help(speedtest, "Do speed test", "Usage: speedtest\n");
int cmd_speedtest(int argc, const char **argv) {
	uint32_t start;
	uint32_t test_time;

	start = time_ms();
	sdmmc_blk_write(&test_card, (uint8_t *) (dram.memory_base), 0,
			CONFIG_SDMMC_SPEED_TEST_SIZE);
	test_time = time_ms() - start;
	printk_info("SDMMC: Write speedtest %uKB in %ums at %uKB/S\n", (CONFIG_SDMMC_SPEED_TEST_SIZE * 512) / 1024, test_time, (CONFIG_SDMMC_SPEED_TEST_SIZE * 512) / test_time);

	start = time_ms();
	sdmmc_blk_read(&test_card, (uint8_t *) (dram.memory_base), 0,
		       CONFIG_SDMMC_SPEED_TEST_SIZE);
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
	sunxi_ccu_t ccu;
	axp_pmu_t primary_pmu;
	axp_pmu_t secondary_pmu;
	sunxi_i2c_t i2c;
	sunxi_remoteproc_t e906;

	arm32_dcache_enable();
	arm32_icache_enable();

	show_banner();
	if (sunxi_remoteproc_dt_read_alias(&e906, "e906", NULL) != DRIVER_OK) {
		printk_error("RISC-V E906: invalid devicetree configuration\n");
		return -1;
	}
	if (sunxi_i2c_dt_read_alias(&i2c, "i2c0") != DRIVER_OK ||
	    sunxi_pmu_dt_read_alias(&primary_pmu, "pmu0", &i2c) != DRIVER_OK ||
	    sunxi_pmu_dt_read_alias(&secondary_pmu, "pmu1", &i2c) != DRIVER_OK ||
	    sunxi_sdhci_dt_read_alias(&test_mmc, "mmc2") != DRIVER_OK) {
		printk_error("Board: invalid devicetree configuration\n");
		return -1;
	}

	if (sunxi_ccu_dt_read(&ccu) != DRIVER_OK) {
		printk_error("CCU: invalid devicetree configuration\n");
		return -1;
	}

	sunxi_clk_init(&ccu);

	set_rpio_power_mode();

	sunxi_clk_dump(&ccu);

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
	pmu_axp2202_set_vol(&primary_pmu, "cldo1", 2100, 1);

	pmu_axp2202_dump(&primary_pmu);
	pmu_axp1530_dump(&secondary_pmu);

	sun55iw3_clk_set_cpu_pll(&ccu, 1416);

	if (sunxi_remoteproc_reset(&e906) != DRIVER_OK) {
		printk_error("RISC-V E906: reset failed\n");
		return -1;
	}

	/* Initialize the DRAM and enable memory management unit (MMU). */
	if (sunxi_dram_dt_read_alias(&dram, "dram0", NULL, NULL) != DRIVER_OK) {
		printk_error("DRAM: invalid devicetree configuration\n");
		return -1;
	}
	uint32_t dram_size = sunxi_dram_init(&dram);

	printk_debug("DRAM Size = %dM\n", dram_size);

	sunxi_clk_dump(&ccu);

	arm32_mmu_enable(dram.memory_base, dram_size);

	sunxi_clk_dump(&ccu);

	/* Initialize the SD host controller. */
	if (sunxi_sdhci_init(&test_mmc) != 0) {
		printk_error("SMHC: %s controller init failed\n", test_mmc.name);
	} else {
		printk_info("SMHC: %s controller initialized\n", test_mmc.name);
	}

	/* Initialize the SD card and check if initialization is successful. */
	if (sdmmc_init(&test_card, &test_mmc) != 0) {
		printk_warning("SMHC: init failed\n");
	} else {
		printk_debug("Card OK!\n");
	}

	syterkit_shell_attach(commands);

	return 0;
}
