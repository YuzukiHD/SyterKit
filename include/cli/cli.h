/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __CLI_H__
#define __CLI_H__

#include <cli/cli_config.h>

/**
 * @brief Set the shell prompt string.
 *
 * The caller must keep @p str valid while the shell is running.
 *
 * @param[in] str Null-terminated prompt string.
 */
void msh_set_prompt(char *str);

/**
 * @brief Read one edited command line from the console.
 *
 * @param[out] cmdline Buffer receiving the null-terminated command line. It
 *                     must hold at least MSH_CMDLINE_CHAR_MAX bytes.
 * @return Number of characters read, excluding the terminating null byte.
 */
int msh_get_cmdline(char *cmdline);

/**
 * @brief Parse one command from a shell input line.
 *
 * @param[in] cmdline Input line to parse.
 * @param[out] argvbuf Storage for the parsed, null-terminated arguments.
 * @param[out] pargc Number of parsed arguments.
 * @param[out] pargv Array receiving pointers into @p argvbuf.
 * @return Position at which parsing of the next command should begin, the
 *         original @p cmdline when parsing is complete, or `NULL` on error.
 */
const char *msh_parse_line(const char *cmdline, char *argvbuf, int *pargc, char **pargv);

/** @brief Shell command descriptor. */
typedef struct msh_command_entry {
	const char *name; /**< Command name. */
	int (*func)(int argc, const char **argv); /**< Command handler. */
	const char *description; /**< One-line command description. */
	const char *usage; /**< Detailed usage text. */
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

/**
 * @brief Execute a command from a command table.
 *
 * @param[in] cmdp Command table to search.
 * @param[in] argc Number of entries in @p argv.
 * @param[in] argv Command argument array.
 * @return Command handler result, or -1 when the command is not found.
 */
int msh_do_command(const msh_command_entry *cmdp, int argc, const char **argv);

/**
 * @brief Print the names and descriptions in a command table.
 *
 * @param[in] cmdlist Null-terminated command table.
 */
void msh_print_cmdlist(const msh_command_entry *cmdlist);

/**
 * @brief Look up usage text for a command.
 *
 * @param[in] cmdlist Null-terminated command table to search.
 * @param[in] cmdname Command name to find.
 * @return Usage text for the command, or `NULL` if it is not present.
 */
const char *msh_get_command_usage(const msh_command_entry *cmdlist, const char *cmdname);

#endif /*__CLI_H__*/
