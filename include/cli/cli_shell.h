/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __CLI_SHELL_H__
#define __CLI_SHELL_H__

#include <cli/cli.h>
#include <cli/cli_termesc.h>

/**
 * @brief Run the interactive shell with an optional user command table.
 *
 * @param[in] cmdlist Null-terminated user command table, or `NULL` to expose
 *                    only built-in commands.
 * @return This function normally does not return.
 */
int syterkit_shell_attach(const msh_command_entry *cmdlist);

#endif //__CLI_SHELL_H__
