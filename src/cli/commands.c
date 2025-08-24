/* SPDX-License-Identifier: GPL-2.0+ */
#include <string.h>
#include <sstdlib.h>

#include <log.h>

#include "cli.h"
#include "cli_config.h"
#include "cli_history.h"
#include "cli_termesc.h"

static int cmd_echo(int argc, const char **argv) {
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

static int cmd_ls(int argc, const char **argv) {
	uart_puts("SyterKit not Support ls command. No file system mounted\n");
	return 0;
}

static int cmd_hexdump(int argc, const char **argv) {
	if (argc != 3) {
		printk(LOG_LEVEL_MUTE, "Usage: hexdump [address] [length]\n");
		return 1;
	}

	uint32_t start_addr = strtol(argv[1], NULL, 0);
	uint32_t len = strtol(argv[2], NULL, 0);

	dump_hex(start_addr, len);

	return 0;
}

static int cmd_read32(int argc, const char **argv) {
	if (argc != 2) {
		printk(LOG_LEVEL_MUTE, "Usage: read32 [address]\n");
		return 1;
	}

	uint32_t ptr = (uint32_t) simple_strtoul(argv[1], NULL, 16);
	uint32_t value = read32(ptr);

	printk(LOG_LEVEL_MUTE, "Value at address 0x%08x: 0x%08X\n", ptr, value);

	return 0;
}

static int cmd_write32(int argc, const char **argv) {
	if (argc < 3) {
		printk(LOG_LEVEL_MUTE, "Usage: write32 [address] [data]\n");
		return -1;
	}
	uint32_t addr = (uint32_t) simple_strtoul(argv[1], NULL, 16);
	uint32_t data = (uint32_t) simple_strtoul(argv[2], NULL, 16);
	write32(addr, data);
	printk(LOG_LEVEL_MUTE, "Wrote 0x%08x to address 0x%08x\n", data, addr);
	return 0;
}

static int cmd_history(int argc, const char **argv) {
	for (int i = get_history_count(); i >= 0; i--) {
		uart_puts(history_get(i));
		uart_putchar('\n');
	}
	return 0;
}

static int cmd_help(int argc, const char **argv) {
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

/* 
    Built-in command list.
    Each entry contains command name, corresponding function, description, and usage.
*/
const msh_command_entry msh_builtin_commands[] = {
		{"help", cmd_help, "display help for available commands",
		 "Usage: help [command]\n"
		 "    Displays help for 'command', or all commands and their\n"
		 "    short descriptions.\n"},
		{"echo", cmd_echo, "echo all arguments separated by a whitespace it can show args", "Usage: echo [string ...]\n"},
		{"history", cmd_history, "show all history command", "Usage: history\n"},
		{"hexdump", cmd_hexdump, "dumps memory region in hex", "Usage: hexdump [address] [length]\n"},
		{"read32", cmd_read32, "read 32-bits value from device reg", "Usage: read32 [address]\n"},
		{"write32", cmd_write32, "write 32-bits value to device reg", "Usage: write32 [address] [data]\n"},
		{"ls", cmd_ls, "linux nerd compatible", "Usage: ls\n"},
		msh_command_end,
};

/* 
    Find the command entry in the command list.
    Parameters:
        cmdlist: The command list to search.
        name: The name of the command to find.
    Returns: A pointer to the command entry if found, or NULL otherwise.
*/
static const msh_command_entry *find_command_entry(const msh_command_entry *cmdlist, const char *name) {
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

/* 
    Execute a command from the command list.
    Parameters:
        cmdlist: The command list to execute from.
        argc: The number of arguments.
        argv: An array of argument strings.
    Returns: The result of the command execution.
*/
int msh_do_command(const msh_command_entry *cmdlist, int argc, const char **argv) {
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

/* 
    Print the list of commands with their descriptions.
    Parameters:
        cmdlist: The command list to print.
*/
void msh_print_cmdlist(const msh_command_entry *cmdlist) {
	int i, j;
	const int indent = 10;

	i = 0;
	while (cmdlist[i].name != NULL) {
		uart_puts("    ");
		uart_puts(cmdlist[i].name);
		for (j = indent - strlen(cmdlist[i].name); j > 0; j--) { uart_putchar(' '); }
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

/* 
    Get the usage of a specific command.
    Parameters:
        cmdlist: The command list to search.
        cmdname: The name of the command to get the usage for.
    Returns: The usage string for the command, or NULL if not found.
*/
const char *msh_get_command_usage(const msh_command_entry *cmdlist, const char *cmdname) {
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
