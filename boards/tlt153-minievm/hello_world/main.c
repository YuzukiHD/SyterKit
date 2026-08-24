/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <backtrace.h>
#include <log.h>
#include <drivers/clk/clk.h>
#include <mmu.h>
#include <malloc.h>
#include <stdlib.h>

#include <cli/cli.h>
#include <cli/cli_shell.h>
#include <cli/cli_termesc.h>

#include <drivers/mmc/sdcard.h>

#include <drivers/dram/dram.h>
#include <dt-compatible/dram-dt.h>
#include <drivers/gpio/gpio.h>
#include <drivers/i2c/i2c.h>
#include <drivers/mtd/spi-nand.h>
#include <drivers/soc/sid.h>
#include <drivers/spi/spi.h>
#include <drivers/serial/serial.h>
#include <dt-compatible/dma-dt.h>
#include <dt-compatible/mmc-dt.h>
#include <dt-compatible/spi-nand-dt.h>
#include <dt-compatible/spi-dt.h>
#include <string.h>

extern sunxi_serial_t uart_dbg;

static sunxi_sdhci_t sdhci0 = { 0 };
static sdmmc_pdata_t mmc_card = { 0 };
static sunxi_dram_t dram;

#define CONFIG_HEAP_BASE (0x44800000)
#define CONFIG_HEAP_SIZE (16 * 1024 * 1024)

#define CONFIG_SDMMC_SPEED_TEST_SIZE 1024 // (unit: 512B sectors)

msh_declare_command(reload);
msh_define_help(reload, "rescan TF Card and reload DTB, Kernel zImage", "Usage: reload\n");
int cmd_reload(int argc, const char **argv)
{
	if (sdmmc_init(&mmc_card, &sdhci0) != 0) {
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
	sdmmc_blk_read(&mmc_card, (uint8_t *)(dram.memory_base), 0, 1024);
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
	memset((void *)dram.memory_base, 0x5a, 0x2000);
	memcpy((void *)dram.memory_base, argv[1], strlen(argv[1]));

	start = time_ms();
	sdmmc_blk_write(&mmc_card, (uint8_t *)(dram.memory_base), 0, 1024);
	test_time = time_ms() - start;
	printk_debug("SDMMC: speedtest %uKB in %ums at %uKB/S\n", (CONFIG_SDMMC_SPEED_TEST_SIZE * 512) / 1024, test_time, (CONFIG_SDMMC_SPEED_TEST_SIZE * 512) / test_time);
	return 0;
}

msh_declare_command(bt);
msh_define_help(bt, "backtrace test", "Usage: bt\n");
int cmd_bt(int argc, const char **argv)
{
	dump_stack();
	return 0;
}

static void __attribute__((noinline)) cmd_fault_undefined(void)
{
	__asm__ volatile(".hword 0xde00" ::: "memory");
}

static void __attribute__((noinline)) cmd_fault_swi(void)
{
	__asm__ volatile("svc #0" ::: "memory");
}

static void __attribute__((noinline)) cmd_fault_prefetch(void)
{
	void (*entry)(void) = (void (*)(void))(uintptr_t)0xdead0001U;

	entry();
}

static void __attribute__((noinline)) cmd_fault_data_read(void)
{
	volatile uint32_t *address = (volatile uint32_t *)(uintptr_t)0xdead0000U;
	uint32_t value = *address;

	(void)value;
}

static void __attribute__((noinline)) cmd_fault_data_write(void)
{
	volatile uint32_t *address = (volatile uint32_t *)(uintptr_t)0xdead0000U;

	*address = 0xdeadbeefU;
}

msh_declare_command(fault);
msh_define_help(fault, "trigger an ARM exception for testing", "Usage: fault <undefined|swi|prefetch|data-read|data-write>\n");
int cmd_fault(int argc, const char **argv)
{
	if (argc != 2) {
		printk_error("Usage: fault <undefined|swi|prefetch|data-read|data-write>\n");
		return -1;
	}

	if (strcmp(argv[1], "undefined") == 0)
		cmd_fault_undefined();
	else if (strcmp(argv[1], "swi") == 0)
		cmd_fault_swi();
	else if (strcmp(argv[1], "prefetch") == 0)
		cmd_fault_prefetch();
	else if (strcmp(argv[1], "data-read") == 0)
		cmd_fault_data_read();
	else if (strcmp(argv[1], "data-write") == 0)
		cmd_fault_data_write();
	else {
		printk_error("fault: unknown type '%s'\n", argv[1]);
		return -1;
	}

	return 0;
}

msh_declare_command(reset);
msh_define_help(reset, "reset the board", "Usage: reset\n");
int cmd_reset(int argc, const char **argv)
{
	sys_reset();
	return 0;
}

msh_declare_command(dram);
msh_define_help(dram, "dump trained dram param", "Usage: dump_dram_param\n");
int cmd_dram(int argc, const char **argv)
{
	printk_info("Trainned DRAM PARAM:\n");
	for (size_t i = 0; i < 32; i += 4) {
		printk_info(" 0x%08x 0x%08x 0x%08x 0x%08x\n", dram.parameters[i], dram.parameters[i + 1], dram.parameters[i + 2], dram.parameters[i + 3]);
	}
	return 0;
}

const msh_command_entry commands[] = {
	msh_define_command(bt),	   msh_define_command(dram), msh_define_command(fault), msh_define_command(reload),
	msh_define_command(reset), msh_define_command(read), msh_define_command(write), msh_command_end,
};

int main(void)
{
	sunxi_dma_t dma;
	spi_nand_t nand;
	sunxi_spi_t spi;

	if (sunxi_serial_init_stdout() != 0)
		return -1;

	show_banner();
	if (sunxi_sdhci_dt_read_alias(&sdhci0, "mmc0") != DRIVER_OK) {
		printk_error("SMHC: invalid devicetree configuration\n");
		return -1;
	}
	if (sunxi_dma_dt_read_alias(&dma, "dma0") != DRIVER_OK || sunxi_spi_dt_read_alias(&spi, "spi0", &dma) != DRIVER_OK ||
	    spi_nand_dt_read_alias(&nand, "spi-nand0", &spi) != DRIVER_OK) {
		printk_error("SPI: invalid devicetree configuration\n");
		return -1;
	}

	sunxi_clk_init();

	if (sunxi_dram_dt_read_alias(&dram, "dram0") != DRIVER_OK) {
		printk_error("DRAM: invalid devicetree configuration\n");
		return -1;
	}
	uint32_t dram_size = sunxi_dram_init(&dram);

	arm32_mmu_enable(dram.memory_base, dram_size);

	/* Initialize the small memory allocator. */
	malloc_init(CONFIG_HEAP_BASE, CONFIG_HEAP_SIZE);

	printk_info("Hello World!\n");

	/* Initialize the SD host controller. */
	if (sunxi_sdhci_init(&sdhci0) != 0) {
		printk_error("SMHC: %s controller init failed\n", sdhci0.name);
	} else {
		printk_info("SMHC: %s controller initialized\n", sdhci0.name);
		if (sdmmc_init(&mmc_card, &sdhci0) != 0) {
			printk_error("SMHC: init failed\n");
		}
	}

	if (sunxi_spi_init(&spi) != 0) {
		printk_error("SPI: init failed\n");
	} else {
		printk_info("SPI controller initialized\n");
		if (spi_nand_detect(&nand) != 0)
			printk_error("SPI: SPI-NAND init failed\n");
	}

	spi_nand_read(&nand, (uint8_t *)dram.memory_base, 0x0, 0x100);

	dump_hex(dram.memory_base, 0x100);

	syterkit_shell_attach(commands);

	abort();

	return 0;
}
