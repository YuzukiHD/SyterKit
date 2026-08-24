/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>
#include <drivers/clk/clk.h>

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
#include <dt-compatible/remoteproc-dt.h>
#include <drivers/mmc/sdcard.h>
#include <drivers/soc/sid.h>
#include <drivers/spi/spi.h>
#include <drivers/serial/serial.h>
#include <string.h>

static sunxi_dram_t dram;

extern sunxi_serial_t uart_dbg;

#define CONFIG_SDMMC_SPEED_TEST_SIZE 1024 // (unit: 512B sectors)

static sdmmc_pdata_t test_card;
static sunxi_sdhci_t test_mmc;

msh_declare_command(reload);
msh_define_help(reload, "rescan TF Card and reload DTB, Kernel zImage", "Usage: reload\n");
int cmd_reload(int argc, const char **argv)
{
	if (sdmmc_init(&test_card, &test_mmc) != 0) {
		printk_error("SMHC: init failed\n");
		return 0;
	}
	return 0;
}

msh_declare_command(read);
msh_define_help(read, "test", "Usage: read\n");
int cmd_read(int argc, const char **argv)
{
	uint32_t start;
	uint32_t test_time;

	printk_debug("Clear Buffer data\n");
	memset((void *)dram.memory_base, 0x00, 0x2000);
	dump_hex(dram.memory_base, 0x100);

	printk_debug("Read data to buffer data\n");

	start = time_ms();
	sdmmc_blk_read(&test_card, (uint8_t *)(dram.memory_base), 0, 1024);
	test_time = time_ms() - start;
	printk_debug("SDMMC: speedtest %uKB in %ums at %uKB/S\n", (CONFIG_SDMMC_SPEED_TEST_SIZE * 512) / 1024, test_time, (CONFIG_SDMMC_SPEED_TEST_SIZE * 512) / test_time);
	dump_hex(dram.memory_base, 0x100);
	return 0;
}

msh_declare_command(write);
msh_define_help(write, "test", "Usage: write\n");
int cmd_write(int argc, const char **argv)
{
	uint32_t start;
	uint32_t test_time;

	printk_debug("Set Buffer data\n");
	memset((void *)dram.memory_base, 0x00, 0x2000);
	memcpy((void *)dram.memory_base, argv[1], strlen(argv[1]));

	start = time_ms();
	sdmmc_blk_write(&test_card, (uint8_t *)(dram.memory_base), 0, 1024);
	test_time = time_ms() - start;
	printk_debug("SDMMC: speedtest %uKB in %ums at %uKB/S\n", (CONFIG_SDMMC_SPEED_TEST_SIZE * 512) / 1024, test_time, (CONFIG_SDMMC_SPEED_TEST_SIZE * 512) / test_time);
	return 0;
}

const msh_command_entry commands[] = {
	msh_define_command(reload),
	msh_define_command(read),
	msh_define_command(write),
	msh_command_end,
};

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
		printk_error("RISC-V E906: invalid devicetree configuration\n");
		return -1;
	}
	if (sunxi_i2c_dt_read_alias(&i2c, "i2c0") != DRIVER_OK || pmu_axp2202_config(&axp2202, &i2c) != DRIVER_OK || pmu_axp1530_config(&axp1530, &i2c) != DRIVER_OK ||
	    sunxi_sdhci_dt_read_alias(&test_mmc, "mmc0") != DRIVER_OK) {
		printk_error("Board: invalid devicetree configuration\n");
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

	sun55iw3_clk_set_cpu_pll(1416);

	if (sunxi_remoteproc_reset(&e906) != DRIVER_OK) {
		printk_error("RISC-V E906: reset failed\n");
		return -1;
	}

	/* Initialize the DRAM and enable memory management unit (MMU). */
	dram.pmu = &axp2202;
	dram.pmu_aux = &axp1530;
	if (sunxi_dram_dt_read_alias(&dram, "dram0") != DRIVER_OK) {
		printk_error("DRAM: invalid devicetree configuration\n");
		return -1;
	}
	uint32_t dram_size = sunxi_dram_init(&dram);

	printk_debug("DRAM Size = %dM\n", dram_size);

	sunxi_clk_dump();

	arm32_mmu_enable(dram.memory_base, dram_size);

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
