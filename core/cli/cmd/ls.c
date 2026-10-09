/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file ls.c
 * @brief Shell command `ls`.
 */
#include <stddef.h>
#include <cli/cli.h>
#include <uart.h>

msh_define_help(ls, "linux nerd compatible",
	"Usage: ls\n");

/**
 * @brief Report that filesystem listing is unavailable in this firmware.
 * @param[in] argc Number of command arguments (unused).
 * @param[in] argv Command argument vector (unused).
 * @return Always zero after printing the diagnostic.
 */
int cmd_ls(int argc, const char **argv)
{
	uart_puts("SyterKit not Support ls command. No file system mounted\n");
	return 0;
}
