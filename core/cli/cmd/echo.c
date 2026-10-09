/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file echo.c
 * @brief Shell command `echo`.
 */
#include <stddef.h>
#include <cli/cli.h>
#include <uart.h>

msh_define_help(echo, "echo all arguments separated by a whitespace it can show args",
	"Usage: echo [string ...]\n");

/**
 * @brief Print each command-line argument separated by one space.
 *
 * The command deliberately ignores the command name in @p argv[0].  A
 * trailing space is emitted after every argument to preserve the shell's
 * historical output format.
 *
 * @param[in] argc Number of entries in @p argv.
 * @param[in] argv Null-terminated command argument vector.
 * @return Always zero; this command has no failure mode.
 */
int cmd_echo(int argc, const char **argv)
{
	int i;
	if (argc < 1) {
		return 0;
	}
	for (i = 1; i < argc; i++) {
		uart_puts(argv[i]);
		uart_putchar(' ');
	}
	uart_putchar('\n');
	return 0;
}
