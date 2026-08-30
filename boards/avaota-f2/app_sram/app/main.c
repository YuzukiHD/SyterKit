/* SPDX-License-Identifier: GPL-2.0+ */

#include <drivers/serial/serial.h>

#include <stdbool.h>
#include <malloc.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <types.h>

#include <dt-bindings/soc/sun252iw1.h>
#include <log.h>
#include <drivers/clk/clk.h>

#include <drivers/dma/dma.h>
#include <drivers/dram/dram.h>
#include <dt-compatible/dram-dt.h>
#include <drivers/gpio/gpio.h>
#include <drivers/i2c/i2c.h>
#include <drivers/mmc/sdcard.h>
#include <drivers/mmc/sdhci.h>
#include <drivers/mtd/spif-nor.h>
#include <drivers/pmu/axp.h>
#include <drivers/spif/spif.h>
#include <dt-compatible/i2c-dt.h>
#include <dt-compatible/mmc-dt.h>
#include <dt-compatible/spif-dt.h>
#include <dt-compatible/spif-nor-dt.h>

#include <common.h>

#include <cli/cli.h>
#include <cli/cli_shell.h>
#include <cli/cli_termesc.h>

#include <e907/sysmap.h>

#define AVAOTA_F2_HEAP_BASE	  0x40800000U
#define AVAOTA_F2_HEAP_SIZE	  (16U * 1024U * 1024U)
#define AVAOTA_F2_FLASH_READ_BASE 0x42000000U
#define AVAOTA_F2_SD_READ_BASE	  0x42100000U

#define CONFIG_SDMMC_SPEED_TEST_SIZE 102400 * 4

sunxi_dram_t dram = { 0 };
sunxi_sdhci_t sdhci0 = { 0 };
sdmmc_pdata_t sdmmc0 = { 0 };

static void sunxi_pmc_config(void)
{
	if (!(readl(SUNXI_RTC_PMC_BYPASS_STATUS) & BIT(0))) {
		/* if PMC bypass, restore all IO to GPIO */
		writel(BIT(0) | BIT(1) | BIT(2) | BIT(5), SUNXI_RTC_IOMODE_CTL);
	}
}

msh_declare_command(speedtest);
msh_define_help(speedtest, "Do speed test", "Usage: speedtest\n");
int cmd_speedtest(int argc, const char **argv)
{
	uint32_t start;
	uint32_t test_time;

	memset((void *)(dram.memory_base), 0x5a, CONFIG_SDMMC_SPEED_TEST_SIZE * 512);

	start = time_ms();
	sdmmc_blk_write(&sdmmc0, (uint8_t *)(dram.memory_base), 0, CONFIG_SDMMC_SPEED_TEST_SIZE);
	test_time = time_ms() - start;
	pr_info("SDMMC: Write speedtest %uKB in %ums at %uKB/S\n", (CONFIG_SDMMC_SPEED_TEST_SIZE * 512) / 1024,
		test_time, (CONFIG_SDMMC_SPEED_TEST_SIZE * 512) / test_time);

	start = time_ms();
	sdmmc_blk_read(&sdmmc0, (uint8_t *)(dram.memory_base), 0, CONFIG_SDMMC_SPEED_TEST_SIZE);
	test_time = time_ms() - start;
	pr_info("SDMMC: Read speedtest %uKB in %ums at %uKB/S\n", (CONFIG_SDMMC_SPEED_TEST_SIZE * 512) / 1024,
		test_time, (CONFIG_SDMMC_SPEED_TEST_SIZE * 512) / test_time);

	return 0;
}

const msh_command_entry commands[] = {
	msh_define_command(speedtest),
	msh_command_end,
};

int main(void)
{
	axp_pmu_t pmu;
	sunxi_i2c_t i2c;
	sunxi_spif_t spif = { 0 };
	spif_nor_t nor = { 0 };
	uint32_t dram_size;

	if (sunxi_serial_init_stdout() != 0)
		return -1;

	show_banner();

	if (sunxi_i2c_dt_read_alias(&i2c, "i2c2") != DRIVER_OK || pmu_axp333_config(&pmu, &i2c) != DRIVER_OK) {
		pr_err("PMU: invalid devicetree configuration\n");
		return -1;
	}

	sysmap_dump_region_info();

	sunxi_clk_dump();

	sunxi_clk_init();

	pr_info("Hello World!\n");

	sunxi_clk_dump();

	sunxi_pmc_config();

	sunxi_i2c_init(&i2c);

	pmu_axp333_init(&pmu);

	pmu_axp333_set_vol(&pmu, "dcdc2", 1500, 1);

	pmu_axp333_dump(&pmu);

	if (sunxi_dram_dt_read_alias(&dram, "dram0") != DRIVER_OK) {
		pr_err("DRAM: invalid devicetree configuration\n");
		return -1;
	}
	dram_size = sunxi_dram_init(&dram);
	if (dram_size == 0U) {
		pr_err("DRAM: initialization failed\n");
		return -1;
	}

	if (malloc_init(AVAOTA_F2_HEAP_BASE, AVAOTA_F2_HEAP_SIZE) != 0) {
		pr_err("Heap: DRAM heap initialization failed\n");
		return -1;
	}

	if (sunxi_spif_dt_read_alias(&spif, "spif0") != DRIVER_OK) {
		pr_err("SPIF: invalid devicetree configuration\n");
		return -1;
	}

	if (spif_nor_dt_read_alias(&nor, "spif-nor0", &spif) != DRIVER_OK) {
		pr_err("SPIF NOR: invalid devicetree configuration\n");
		return -1;
	}

	if (sunxi_spif_init(&spif) != 0) {
		pr_err("SPIF: controller init failed\n");
		return -1;
	}

	if (spif_nor_detect(&nor) != 0) {
		pr_err("SPIF NOR: no supported flash detected\n");
		return -1;
	}

	/* Read the first 1 MiB to DRAM to verify the SPIF data path. */
	uint32_t nor_read_size = 1024U * 1024U;
	uint32_t time_start = time_us();
	uint32_t nor_read_done = spif_nor_read(&nor, (uint8_t *)AVAOTA_F2_FLASH_READ_BASE, 0U, nor_read_size);
	uint32_t time_end = time_us();
	uint32_t delta = time_end - time_start;
	if (delta == 0U)
		delta = 1U;
	pr_info("SPIF NOR: read %uKiB in %uus, %uKiB/s\n", nor_read_done / 1024U, delta,
		(uint32_t)(((uint64_t)nor_read_done * 1000U) / ((uint64_t)delta)));
	dump_hex(AVAOTA_F2_FLASH_READ_BASE, 0x40);

	if (sunxi_sdhci_dt_read_alias(&sdhci0, "mmc0") != DRIVER_OK) {
		pr_err("SMHC0: invalid devicetree configuration\n");
		return -1;
	}

	/* Initialize the SD host controller before probing the card. */
	if (sunxi_sdhci_init(&sdhci0) != 0) {
		pr_err("SMHC: %s controller init failed\n", sdhci0.name);
		return -1;
	} else {
		pr_info("SMHC: %s controller initialized\n", sdhci0.name);
	}

	if (sdmmc_init(&sdmmc0, &sdhci0) == 0) {
		uint32_t sd_read_done = sdmmc_blk_read(&sdmmc0, (uint8_t *)AVAOTA_F2_SD_READ_BASE, 0U, 1U);
		if (sd_read_done == 1U) {
			pr_info("SMHC0: read SD card block 0\n");
			dump_hex(AVAOTA_F2_SD_READ_BASE, 0x40);
		} else {
			pr_warn("SMHC0: SD card block read failed\n");
		}
	} else {
		pr_warn("SMHC0: no SD card detected\n");
	}

	syterkit_shell_attach(commands);

	abort();

	return 0;
}
