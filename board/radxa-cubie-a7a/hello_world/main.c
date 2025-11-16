/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>

#include <common.h>
#include <mmu.h>

#include <mmc/sys-sdhci.h>

#include <sys-dram.h>
#include <sys-sdcard.h>
#include <sys-i2c.h>

#include <cli.h>
#include <cli_shell.h>
#include <cli_termesc.h>

extern sunxi_serial_t uart_dbg;

extern sunxi_i2c_t i2c_pmu;

extern sunxi_sdhci_t sdhci0;

extern uint32_t dram_para[128];

extern void board_common_init(void);

extern int init_DRAM(int type, void *buff);

msh_declare_command(bt);
msh_define_help(bt, "backtrace test", "Usage: bt\n");
int cmd_bt(int argc, const char **argv) {
	dump_stack();
	return 0;
}

msh_declare_command(ddr_test);
msh_define_help(ddr_test, "ddr w/r test", "Usage: ddr_test\n");
int cmd_ddr_test(int argc, const char **argv) {
	dump_hex(SDRAM_BASE, 0x100);
	memset((void *) SDRAM_BASE, 0x5A, 0x2000);
	dump_hex(SDRAM_BASE, 0x100);
	return 0;
}

const msh_command_entry commands[] = {
		msh_define_command(bt),
		msh_define_command(ddr_test),
		msh_command_end,
};

int main(void) {
	sunxi_serial_init(&uart_dbg);

	show_banner();

	board_common_init();

	sunxi_i2c_init(&i2c_pmu);

	sunxi_clk_init();

	sunxi_clk_dump();

	pmu_axp8191_init(&i2c_pmu);

	pmu_axp8191_dump(&i2c_pmu);

	sunxi_dram_init(NULL);

	printk_info("Hello World!\n");

	init_DRAM(0, dram_para);

	syterkit_shell_attach(commands);

	abort();

	return 0;
}