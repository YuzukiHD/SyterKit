/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file history.c
 * @brief Shell command `history`.
 */
#include <stddef.h>
#include <cli/cli.h>
#include <cli/cli_history.h>
#include <uart.h>

msh_define_help(history, "show all history command",
	"Usage: history\n");

/**
 * @brief Print command history from newest entry to oldest entry.
 * @param[in] argc Number of command arguments (unused).
 * @param[in] argv Command argument vector (unused).
 * @return Always zero.
 */
int cmd_history(int argc, const char **argv)
{
	int count = get_history_count();

	for (int i = 0; i < count; i++) {
		const char *line = history_get(i);

		if (line != NULL)
			uart_puts(line);
		uart_putchar('\n');
	}
	return 0;
}
