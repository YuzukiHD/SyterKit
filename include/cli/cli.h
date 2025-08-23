/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __CLI_H__
#define __CLI_H__

#include "cli_config.h"

void msh_set_prompt(char *str);

int msh_get_cmdline(char *cmdline);

const char *msh_parse_line(const char *cmdline, char *argvbuf, int *pargc, char **pargv);

typedef struct msh_command_entry {
	const char *name;
	int (*func)(int argc, const char **argv);
	const char *description;
	const char *usage;
} msh_command_entry;

#define msh_command_end \
	{ 0, 0, 0, 0 }

#define msh_declare_command(name)                \
	int cmd_##name(int argc, const char **argv); \
	extern const char cmd_##name##_desc[];       \
	extern const char cmd_##name##_usage[];

#define msh_define_help(name, desc, usage) \
	const char cmd_##name##_desc[] = desc; \
	const char cmd_##name##_usage[] = usage;

#define msh_define_command(name) \
	{ #name, cmd_##name, cmd_##name##_desc, cmd_##name##_usage }

extern const msh_command_entry msh_builtin_commands[];

extern const msh_command_entry *msh_user_commands;

int msh_do_command(const msh_command_entry *cmdp, int argc, const char **argv);

void msh_print_cmdlist(const msh_command_entry *cmdlist);

const char *msh_get_command_usage(const msh_command_entry *cmdlist, const char *cmdname);

#endif /*__CLI_H__*/
