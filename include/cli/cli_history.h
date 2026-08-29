/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file cli_history.h
 * @brief Command line history storage for the SyterKit CLI.
 */

#ifndef __CLI_HISTORY_H__
#define __CLI_HISTORY_H__

#include <cli/cli_config.h>

/**
 * @brief Get the number of stored command line history entries.
 */
int get_history_count();

/**
 * @brief Append a command line to the history buffer.
 */
void history_append(const char *line);

/**
 * @brief Retrieve a command line history entry.
 */
const char *history_get(int histnum);

#endif // __CLI_HISTORY_H__
