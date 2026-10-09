/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file commands.c
 * @brief Built-in command table and command-table dispatch helpers.
 *
 * The commands themselves live in cmd/, one file each; this file only lists
 * them and looks them up.
 */
#include <stddef.h>
#include <string.h>

#include <cli/cli.h>
#include <uart.h>

msh_declare_command(help);
msh_declare_command(echo);
msh_declare_command(history);
msh_declare_command(hexdump);
msh_declare_command(read32);
msh_declare_command(write32);
msh_declare_command(screenfetch);
msh_declare_command(ls);

/** @brief Built-in command table. */
const msh_command_entry msh_builtin_commands[] = {
	msh_define_command(help),
	msh_define_command(echo),
	msh_define_command(history),
	msh_define_command(hexdump),
	msh_define_command(read32),
	msh_define_command(write32),
	msh_define_command(screenfetch),
	msh_define_command(ls),
	msh_command_end,
};

/**
 * @brief Find a command in a command table.
 *
 * @param cmdlist Command table to search.
 * @param name Command name to find.
 * @return The matching command entry, or `NULL` if none is found.
 */
static const msh_command_entry *find_command_entry(const msh_command_entry *cmdlist, const char *name)
{
	int i = 0;
	while (cmdlist[i].name != NULL) {
		if (strcmp(cmdlist[i].name, name) == 0) {
			return &cmdlist[i];
		} else {
			i++;
		}
	}
	return NULL;
}

/**
 * @brief Execute a command from a command table.
 *
 * @param cmdlist Command table to search.
 * @param argc Number of command arguments.
 * @param argv Array of command arguments.
 * @return The command result, or -1 if the command is invalid or not found.
 */
int msh_do_command(const msh_command_entry *cmdlist, int argc, const char **argv)
{
	const msh_command_entry *cmd_entry;

	if (argc < 1) {
		return -1;
	}

	cmd_entry = find_command_entry(cmdlist, argv[0]);

	if (cmd_entry != NULL) {
		return (cmd_entry->func(argc, argv));
	} else {
		return -1;
	}
}

/**
 * @brief Print a command table and its descriptions.
 *
 * @param cmdlist Command table to print.
 */
void msh_print_cmdlist(const msh_command_entry *cmdlist)
{
	int i, j;
	const int indent = 10;

	i = 0;
	while (cmdlist[i].name != NULL) {
		uart_puts("    ");
		uart_puts(cmdlist[i].name);
		for (j = indent - strlen(cmdlist[i].name); j > 0; j--) {
			uart_putchar(' ');
		}
		uart_puts("- ");
		if (cmdlist[i].description != NULL) {
			uart_puts(cmdlist[i].description);
			uart_puts("\n");
		} else {
			uart_puts("(No description available)\n");
		}
		i++;
	}
	return;
}

/**
 * @brief Get the usage text for a command.
 *
 * @param cmdlist Command table to search.
 * @param cmdname Command name to find.
 * @return The usage text, or `NULL` if the command is not found.
 */
const char *msh_get_command_usage(const msh_command_entry *cmdlist, const char *cmdname)
{
	const msh_command_entry *cmd_entry;

	cmd_entry = find_command_entry(cmdlist, cmdname);
	if (cmd_entry == NULL) {
		return NULL; /* No such command */
	} else if (cmd_entry->usage == NULL) {
		return "No help available.\n";
	} else {
		return cmd_entry->usage;
	}
}
