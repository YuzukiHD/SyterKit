/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <backtrace.h>
#include <log.h>
#include <dt-compatible/ccu-dt.h>

#include <common.h>
#include <mmu.h>
#include <stdlib.h>

#include <drivers/dram.h>
#include <dt-compatible/dram-dt.h>
#include <drivers/i2c.h>
#include <drivers/pmu/axp.h>
#include <dt-compatible/i2c-dt.h>
#include <dt-compatible/pmu-dt.h>

#include <cli/cli.h>
#include <cli/cli_shell.h>
#include <cli/cli_termesc.h>

extern sunxi_serial_t uart_dbg;



extern void board_common_init(void);


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
	sunxi_ccu_t ccu;
	sunxi_dram_t dram;
	axp_pmu_t pmu;
	sunxi_i2c_t i2c;

	show_banner();
	if (sunxi_i2c_dt_read_alias(&i2c, "i2c0") != DRIVER_OK ||
	    sunxi_pmu_dt_read_alias(&pmu, "pmu0", &i2c) != DRIVER_OK) {
		printk_error("PMU: invalid devicetree configuration\n");
		return -1;
	}

	board_common_init();

	sunxi_i2c_init(&i2c);

	if (sunxi_ccu_dt_read(&ccu) != DRIVER_OK) {
		printk_error("CCU: invalid devicetree configuration\n");
		return -1;
	}

	sunxi_clk_init(&ccu);

	sunxi_clk_dump(&ccu);

	pmu_axp8191_init(&pmu);

	pmu_axp8191_dump(&pmu);

	if (sunxi_dram_dt_read_alias(&dram, "dram0", &pmu, NULL) != DRIVER_OK) {
		printk_error("DRAM: invalid devicetree configuration\n");
		return -1;
	}
	sunxi_dram_init(&dram);

	printk_info("Hello World!\n");

	syterkit_shell_attach(commands);

	abort();

	return 0;
}
