/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <types.h>

#include <backtrace.h>
#include <log.h>
#include <dt-compatible/ccu-dt.h>

#include <drivers/clk/clk.h>
#include <drivers/dma/dma.h>
#include <drivers/dram/dram.h>
#include <dt-compatible/dram-dt.h>
#include <drivers/gpio/gpio.h>
#include <drivers/i2c/i2c.h>
#include <drivers/mmc/sdcard.h>
#include <drivers/mmc/sdhci.h>
#include <drivers/mtd/spi-nor.h>
#include <dt-bindings/soc/sun300iw1.h>
#include <drivers/spi/spi.h>
#include <dt-compatible/dma-dt.h>
#include <dt-compatible/mmc-dt.h>
#include <dt-compatible/spi-nor-dt.h>
#include <dt-compatible/spi-dt.h>

#include <common.h>

#include <cli/cli.h>
#include <cli/cli_shell.h>
#include <cli/cli_termesc.h>
#include <string.h>

static sunxi_dram_t dram;

extern sunxi_serial_t uart_dbg;

static sdmmc_pdata_t sd_card;
static sunxi_sdhci_t sdhci0;

#define CONFIG_SDMMC_SPEED_TEST_SIZE 4 * 1024// (unit: 512B sectors)
#define CHUNK_SIZE 0x20000

msh_declare_command(read);
msh_define_help(read, "read SMHC", "Usage: read\n");
int cmd_read(int argc, const char **argv) {
	uint32_t start;
	uint32_t test_time;

	printk_debug("Clear Buffer data\n");
	memset((void *) dram.memory_base, 0xFF, 0x2000);
	dump_hex(dram.memory_base, 0x100);

	printk_debug("Read data to buffer data\n");

	start = time_ms();
	sdmmc_blk_read(&sd_card, (uint8_t *) (dram.memory_base), 0, CONFIG_SDMMC_SPEED_TEST_SIZE);
	test_time = time_ms() - start;
	printk_debug("SDMMC: speedtest %uKB in %ums at %uKB/S\n", (CONFIG_SDMMC_SPEED_TEST_SIZE * 512) / 1024, test_time, (CONFIG_SDMMC_SPEED_TEST_SIZE * 512) / test_time);
	dump_hex(dram.memory_base, 0x100);
	return 0;
}

msh_declare_command(write);
msh_define_help(write, "test", "Usage: write\n");
int cmd_write(int argc, const char **argv) {
	uint32_t start;
	uint32_t test_time;

	printk_debug("Set Buffer data\n");
	memset((void *) dram.memory_base, 0x00, 0x2000);
	memcpy((void *) dram.memory_base, argv[1], strlen(argv[1]));

	start = time_ms();
	sdmmc_blk_write(&sd_card, (uint8_t *) (dram.memory_base), 0, CONFIG_SDMMC_SPEED_TEST_SIZE);
	test_time = time_ms() - start;
	printk_debug("SDMMC: speedtest %uKB in %ums at %uKB/S\n", (CONFIG_SDMMC_SPEED_TEST_SIZE * 512) / 1024, test_time, (CONFIG_SDMMC_SPEED_TEST_SIZE * 512) / test_time);
	return 0;
}

msh_declare_command(load);
msh_define_help(load, "load SMHC", "Usage: load\n");
int cmd_load(int argc, const char **argv) {
	if (sunxi_sdhci_init(&sdhci0) != 0) {
		printk_error("SMHC: %s controller init failed\n", sdhci0.name);
	} else {
		printk_info("SMHC: %s controller initialized\n", sdhci0.name);
	}

	/* Initialize the SD card and check if initialization is successful. */
	if (sdmmc_init(&sd_card, &sdhci0) != 0) {
		printk_warning("SMHC: init failed\n");
	} else {
		printk_debug("Card OK!\n");
	}
	return 0;
}

msh_declare_command(reset);
msh_define_help(reset, "reset test", "Usage: reset\n");
int cmd_reset(int argc, const char **argv) {
	setbits_le32(SUNXI_PRCM_BASE + 0x1c, BIT(3));			/* enable WDT clk */
	writel(0x16aa0000, SUNXI_RTC_WDG_BASE + 0x18);			/* disable WDT */
	writel(0x16aa0000 | BIT(0), SUNXI_RTC_WDG_BASE + 0x08); /* trigger WDT */
	return 0;
}

msh_declare_command(bt);
msh_define_help(bt, "backtrace test", "Usage: bt\n");
int cmd_bt(int argc, const char **argv) {
	dump_stack();
	return 0;
}

const msh_command_entry commands[] = {
		msh_define_command(load),
		msh_define_command(read),
		msh_define_command(write),
		msh_define_command(bt),
		msh_define_command(reset),
		msh_command_end,
};

int main(void) {
	sunxi_ccu_t ccu;
	sunxi_dma_t dma;
	spi_nor_t nor;
	sunxi_spi_t spi;

	show_banner();
	if (sunxi_sdhci_dt_read_alias(&sdhci0, "mmc0") != DRIVER_OK) {
		printk_error("SMHC: invalid devicetree configuration\n");
		return -1;
	}
	if (sunxi_dma_dt_read_alias(&dma, "dma0") != DRIVER_OK ||
	    sunxi_spi_dt_read_alias(&spi, "spi0", &dma) != DRIVER_OK ||
	    spi_nor_dt_read_alias(&nor, "spi-nor0", &spi) != DRIVER_OK) {
		printk_error("SPI: invalid devicetree configuration\n");
		return -1;
	}

	printk_info("Hello World!\n");

	if (sunxi_ccu_dt_read(&ccu) != DRIVER_OK) {
		printk_error("CCU: invalid devicetree configuration\n");
		return -1;
	}

	sunxi_clk_init(&ccu);

	printk_info("CLK init finish\n");

	sunxi_clk_dump(&ccu);

	if (sunxi_dram_dt_read_alias(&dram, "dram0", NULL, NULL) != DRIVER_OK) {
		printk_error("DRAM: invalid devicetree configuration\n");
		return -1;
	}
	sunxi_dram_init(&dram);

	sunxi_spi_init(&spi);

	spi_nor_detect(&nor);

	memset((void *) 0x81000000, 0x0, 0x1000);

	uint32_t time = time_ms();
	spi_nor_read(&nor, (void *) 0x81000000, 0x0, 1024 * 1024 * 4);
	uint32_t time_end = time_ms();

	printk_debug("SPI: speedtest %uKB in %ums at %uKB/S\n", 1024 * 1024 * 4 / 1024, (time_end - time), 1024 * 1024 * 4 / (time_end - time));

	syterkit_shell_attach(commands);

	abort();

	return 0;
}
