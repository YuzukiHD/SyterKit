/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>
#include <drivers/clk/clk.h>

#include <common.h>

#include <cli/cli.h>
#include <cli/cli_shell.h>
#include <cli/cli_termesc.h>

extern sunxi_serial_t uart_dbg;

msh_declare_command(helloworld);

msh_define_help(helloworld, "display helloworld", "Usage: helloworld\n");
int cmd_helloworld(int argc, const char **argv)
{
	printk(LOG_LEVEL_MUTE, "Hello World!\n");
	return 0;
}

const msh_command_entry commands[] = {
	msh_define_command(helloworld),
	msh_command_end,
};

int main(void)
{
	sunxi_clk_init();

	printk_info("Hello World!\n");

	syterkit_shell_attach(commands);

	return 0;
}