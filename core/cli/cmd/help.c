/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file help.c
 * @brief Shell command `help`.
 */
#include <stddef.h>
#include <cli/cli.h>

msh_define_help(help, "display help for available commands",
	"Usage: help [command]\n"
	"    Displays help for 'command', or all commands and their\n"
	"    short descriptions.\n");

/**
 * @brief Display all command help or the usage for one named command.
 *
 * User commands are searched before built-in commands when a name is given,
 * allowing an application command to override the corresponding help text.
 *
 * @param[in] argc Number of command arguments.
 * @param[in] argv Argument vector; @c argv[1], when present, is the name.
 * @return Always zero; unknown names are reported to the console.
 */
int cmd_help(int argc, const char **argv)
{
	if (argc == 1) {
		msh_print_cmdlist(msh_builtin_commands);
		msh_print_cmdlist(msh_user_commands);
	} else {
		const char *usage;

		usage = msh_get_command_usage(msh_user_commands, argv[1]);
		if (usage == NULL) {
			usage = msh_get_command_usage(msh_builtin_commands, argv[1]);
		}

		if (usage == NULL) {
			uart_puts("No such command: '");
			uart_puts(argv[1]);
			uart_puts("'\n");
		} else {
			uart_puts(usage);
		}
	}
	return 0;
}
