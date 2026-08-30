/* SPDX-License-Identifier: GPL-2.0+ */

#include <cli/cli_config.h>
#include <cli/cli_history.h>

#include "syter_test.h"

void test_case_main(const char *case_dir)
{
	char data[TEST_DATA_MAX];
	char *entries[MSH_CMD_HISTORY_MAX + 4];
	char *cursor;
	int entry_count = 0;
	int length;

	length = test_load_data(case_dir, "data/commands.txt", data, sizeof(data));
	TEST_ASSERT(length > 0);
	if (length <= 0)
		return;

	cursor = data;
	while (*cursor) {
		char *end = cursor;

		while (*end && *end != '\n')
			end++;
		if (*end)
			*end++ = '\0';
		if (*cursor) {
			entries[entry_count++] = cursor;
			history_append(cursor);
		}
		cursor = end;
	}

	TEST_ASSERT(entry_count > MSH_CMD_HISTORY_MAX);
	TEST_EQ(MSH_CMD_HISTORY_MAX, get_history_count());
	for (int age = 0; age < MSH_CMD_HISTORY_MAX; age++)
		TEST_STREQ(entries[entry_count - age - 1], history_get(age));
	TEST_ASSERT(history_get(MSH_CMD_HISTORY_MAX) == NULL);
	TEST_ASSERT(history_get(-1) == NULL);
}
