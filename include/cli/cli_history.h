/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __CLI_HISTORY_H__
#define __CLI_HISTORY_H__

#include "cli_config.h"

int get_history_count();

void history_append(const char *line);

const char *history_get(int histnum);

#endif// __CLI_HISTORY_H__
