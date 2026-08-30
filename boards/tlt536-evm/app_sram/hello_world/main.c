/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>
#include <drivers/clk/clk.h>
#include <dt-compatible/dt-common.h>
#include <mmu.h>

#include <cli/cli.h>
#include <cli/cli_shell.h>
#include <cli/cli_termesc.h>

#include <drivers/dram/dram.h>
#include <drivers/gpio/gpio.h>
#include <drivers/i2c/i2c.h>
#include <drivers/pmu/axp.h>
#include <drivers/mmc/sdcard.h>
#include <drivers/sid/sid.h>
#include <drivers/spi/spi.h>
#include <drivers/serial/serial.h>
#include <dt-compatible/i2c-dt.h>
#include <dt-compatible/mmc-dt.h>
#include <string.h>

extern sunxi_serial_t uart_dbg;

static sunxi_sdhci_t sdhci0 = { 0 };
static sdmmc_pdata_t mmc_card = { 0 };
static sunxi_dram_t dram;

#define CONFIG_SDMMC_SPEED_TEST_SIZE 1024 // (unit: 512B sectors)

msh_declare_command(reload);
msh_define_help(reload, "rescan TF Card and reload DTB, Kernel zImage", "Usage: reload\n");
int cmd_reload(int argc, const char **argv)
{
	if (sdmmc_init(&mmc_card, &sdhci0) != 0) {
		pr_err("SMHC: init failed\n");
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

	pr_debug("Clear Buffer data\n");
	memset((void *)dram.memory_base, 0x00, 0x2000);
	dump_hex(dram.memory_base, 0x100);

	pr_debug("Read data to buffer data\n");

	start = time_ms();
	sdmmc_blk_read(&mmc_card, (uint8_t *)dram.memory_base, 0, 1024);
	test_time = time_ms() - start;
	pr_debug("SDMMC: speedtest %uKB in %ums at %uKB/S\n", (CONFIG_SDMMC_SPEED_TEST_SIZE * 512) / 1024, test_time, (CONFIG_SDMMC_SPEED_TEST_SIZE * 512) / test_time);
	dump_hex(dram.memory_base, 0x100);
	return 0;
}

msh_declare_command(write);
msh_define_help(write, "test", "Usage: write\n");
int cmd_write(int argc, const char **argv)
{
	uint32_t start;
	uint32_t test_time;

	pr_debug("Set Buffer data\n");
	memset((void *)dram.memory_base, 0x00, 0x2000);
	memcpy((void *)dram.memory_base, argv[1], strlen(argv[1]));

	start = time_ms();
	sdmmc_blk_write(&mmc_card, (uint8_t *)dram.memory_base, 0, 1024);
	test_time = time_ms() - start;
	pr_debug("SDMMC: speedtest %uKB in %ums at %uKB/S\n", (CONFIG_SDMMC_SPEED_TEST_SIZE * 512) / 1024, test_time, (CONFIG_SDMMC_SPEED_TEST_SIZE * 512) / test_time);
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
	axp_pmu_t pmu;
	sunxi_i2c_t i2c;

	if (sunxi_serial_init_stdout() != 0)
		return -1;

	show_banner();

	if (syterkit_dt_read_reg_alias("memory0", &dram.memory_base, &dram.memory_size) != 0) {
		pr_err("DRAM: invalid devicetree memory window\n");
		return -1;
	}

	if (sunxi_sdhci_dt_read_alias(&sdhci0, "mmc0") != DRIVER_OK) {
		pr_err("SMHC: invalid devicetree configuration\n");
		return -1;
	}

	if (sunxi_i2c_dt_read_alias(&i2c, "i2c0") != DRIVER_OK || pmu_axp2202_config(&pmu, &i2c) != DRIVER_OK) {
		pr_err("PMU: invalid devicetree configuration\n");
		return -1;
	}

	sunxi_gpio_power_mode_init();

	sunxi_i2c_init(&i2c);

	pmu_axp2202_init(&pmu);

	pmu_axp2202_dump(&pmu);

	arm32_dcache_enable();

	arm32_icache_enable();

	sunxi_clk_init();

	sunxi_clk_dump();

	pr_info("Hello World!\n");

	/* Initialize the SD host controller. */
	if (sunxi_sdhci_init(&sdhci0) != 0) {
		pr_err("SMHC: %s controller init failed\n", sdhci0.name);
	} else {
		pr_info("SMHC: %s controller initialized\n", sdhci0.name);
	}

	/* Initialize the SD card and check if initialization is successful. */
	if (sdmmc_init(&mmc_card, &sdhci0) != 0) {
		pr_warn("SMHC: init failed\n");
	} else {
		pr_debug("Card OK!\n");
	}

	syterkit_shell_attach(commands);

	abort();

	return 0;
}
