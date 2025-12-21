/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>
#include <mmu.h>
#include <ff.h>

#include <cli.h>
#include <cli_shell.h>
#include <cli_termesc.h>

#include <mmc/sys-sdcard.h>

#include <sys-dram.h>
#include <sys-gpio.h>
#include <sys-i2c.h>
#include <sys-sid.h>
#include <sys-spi.h>
#include <sys-uart.h>

extern sunxi_serial_t uart_dbg;
extern uint32_t dram_para[96];
extern uint32_t dram_para_trained[96];
extern sunxi_sdhci_t sdhci0;
extern sunxi_spi_t sunxi_spi0;

msh_declare_command(bt);
msh_define_help(bt, "backtrace test", "Usage: bt\n");
int cmd_bt(int argc, const char **argv) {
	dump_stack();
	return 0;
}

msh_declare_command(dram_para);
msh_define_help(dram_para, "dump trained dram param", "Usage: dump_dram_param\n");
int cmd_dram_para(int argc, const char **argv) {
	printk_info("Trainned DRAM PARAM:\n");
	for (size_t i = 0; i < 32; i += 4) {
		printk_info(" 0x%08x 0x%08x 0x%08x 0x%08x\n", dram_para[i], dram_para[i + 1], dram_para[i + 2], dram_para[i + 3]);
	}
	return 0;
}

const msh_command_entry commands[] = {
		msh_define_command(bt),
		msh_define_command(dram_para),
		msh_command_end,
};

int main(void) {
	sunxi_serial_init(&uart_dbg);

	show_banner();

	sunxi_clk_init();

	sunxi_dram_init(dram_para_trained);

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

	syterkit_shell_attach(commands);

	abort();

	return 0;
}