/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <backtrace.h>
#include <log.h>
#include <mmu.h>
#include <malloc.h>
#include <stdlib.h>

#include <cli/cli.h>
#include <cli/cli_shell.h>
#include <cli/cli_termesc.h>

#include <drivers/mmc/sdcard.h>

#include <drivers/dram.h>
#include <drivers/gpio.h>
#include <drivers/i2c.h>
#include <drivers/mtd/spi-nand.h>
#include <drivers/sid.h>
#include <drivers/spi.h>
#include <drivers/serial.h>

extern sunxi_serial_t uart_dbg;
extern uint32_t dram_para[96];
extern uint32_t dram_para_trained[96];
extern sunxi_sdhci_t sdhci0;
extern sunxi_spi_t sunxi_spi0;

#define CONFIG_HEAP_BASE (0x44800000)
#define CONFIG_HEAP_SIZE (16 * 1024 * 1024)

#define CONFIG_SDMMC_SPEED_TEST_SIZE 1024// (unit: 512B sectors)

msh_declare_command(reload);
msh_define_help(reload, "rescan TF Card and reload DTB, Kernel zImage", "Usage: reload\n");
int cmd_reload(int argc, const char **argv) {
	if (sdmmc_init(&card0, &sdhci0) != 0) {
		printk_error("SMHC: init failed\n");
		return 0;
	}
	return 0;
}

msh_declare_command(read);
msh_define_help(read, "test", "Usage: read\n");
int cmd_read(int argc, const char **argv) {
	uint32_t start;
	uint32_t test_time;

	printk_debug("Clear Buffer data\n");
	memset((void *) SDRAM_BASE, 0x00, 0x2000);
	dump_hex(SDRAM_BASE, 0x100);

	printk_debug("Read data to buffer data\n");

	start = time_ms();
	sdmmc_blk_read(&card0, (uint8_t *) (SDRAM_BASE), 0, 1024);
	test_time = time_ms() - start;
	printk_debug("SDMMC: speedtest %uKB in %ums at %uKB/S\n", (CONFIG_SDMMC_SPEED_TEST_SIZE * 512) / 1024, test_time, (CONFIG_SDMMC_SPEED_TEST_SIZE * 512) / test_time);
	dump_hex(SDRAM_BASE, 0x100);
	return 0;
}

msh_declare_command(write);
msh_define_help(write, "test", "Usage: write\n");
int cmd_write(int argc, const char **argv) {
	uint32_t start;
	uint32_t test_time;

	printk_debug("Set Buffer data\n");
	memset((void *) SDRAM_BASE, 0x00, 0x2000);
	memcpy((void *) SDRAM_BASE, argv[1], strlen(argv[1]));

	start = time_ms();
	sdmmc_blk_write(&card0, (uint8_t *) (SDRAM_BASE), 0, 1024);
	test_time = time_ms() - start;
	printk_debug("SDMMC: speedtest %uKB in %ums at %uKB/S\n", (CONFIG_SDMMC_SPEED_TEST_SIZE * 512) / 1024, test_time, (CONFIG_SDMMC_SPEED_TEST_SIZE * 512) / test_time);
	return 0;
}

msh_declare_command(bt);
msh_define_help(bt, "backtrace test", "Usage: bt\n");
int cmd_bt(int argc, const char **argv) {
	dump_stack();
	return 0;
}

msh_declare_command(dram);
msh_define_help(dram, "dump trained dram param", "Usage: dump_dram_param\n");
int cmd_dram(int argc, const char **argv) {
	printk_info("Trainned DRAM PARAM:\n");
	for (size_t i = 0; i < 32; i += 4) {
		printk_info(" 0x%08x 0x%08x 0x%08x 0x%08x\n", dram_para[i], dram_para[i + 1], dram_para[i + 2], dram_para[i + 3]);
	}
	return 0;
}

const msh_command_entry commands[] = {
		msh_define_command(bt),
		msh_define_command(dram),
		msh_define_command(reload),
		msh_define_command(read),
		msh_define_command(write),
		msh_command_end,
};

int main(void) {

	show_banner();

	sunxi_clk_init();

	uint32_t dram_size = sunxi_dram_init(dram_para_trained);
	
	arm32_mmu_enable(SDRAM_BASE, dram_size);

	/* Initialize the small memory allocator. */
	malloc_init(CONFIG_HEAP_BASE, CONFIG_HEAP_SIZE);

	printk_info("Hello World!\n");

	/* Initialize the SD host controller. */
	if (sunxi_sdhci_init(&sdhci0) != 0) {
		printk_error("SMHC: %s controller init failed\n", sdhci0.name);
	} else {
		printk_info("SMHC: %s controller initialized\n", sdhci0.name);
		if (sdmmc_init(&card0, &sdhci0) != 0) {
			printk_error("SMHC: init failed\n");
		}
	}

	if (sunxi_spi_init(&sunxi_spi0) != 0) {
		printk_error("SPI: init failed\n");
	} else {
		printk_info("SPI controller initialized\n");
		if (spi_nand_detect(&sunxi_spi0) != 0)
			printk_error("SPI: SPI-NAND init failed\n");
	}

	spi_nand_read(&sunxi_spi0, (uint8_t *) SDRAM_BASE, 0x0, 0x100);

	dump_hex(SDRAM_BASE, 0x100);

	syterkit_shell_attach(commands);

	abort();

	return 0;
}
