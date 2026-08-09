/* SPDX-License-Identifier: GPL-2.0+ */

#include <stddef.h>

#include <cli/cli.h>

#include "syter_test.h"

int uart_puts(const char *text) {
	(void) text;
	return 0;
}

static char *next_field(char **cursor) {
	char *field = *cursor;
	char *separator = field;

	while (*separator && *separator != '|')
		separator++;
	if (*separator) {
		*separator = '\0';
		*cursor = separator + 1;
	} else {
		*cursor = separator;
	}
	return field;
}

static int decimal(const char *text) {
	int value = 0;

	while (*text >= '0' && *text <= '9')
		value = value * 10 + *text++ - '0';
	return value;
}

void test_case_main(const char *case_dir) {
	char data[TEST_DATA_MAX];
	char argument_buffer[512];
	char *arguments[16];
	char *line;
	int length;

	length = test_load_data(case_dir, "data/vectors.txt", data, sizeof(data));
	TEST_ASSERT(length > 0);
	if (length <= 0)
		return;

	line = data;
	while (*line) {
		char *line_end = line;
		char *cursor;
		char *input;
		char *argc_text;
		const char *next;
		int argc = 0;
		int expected_argc;

		while (*line_end && *line_end != '\n')
			line_end++;
		if (*line_end)
			*line_end++ = '\0';
		if (!*line) {
			line = line_end;
			continue;
		}

		cursor = line;
		input = next_field(&cursor);
		argc_text = next_field(&cursor);
		expected_argc = (*argc_text == '-') ? -1 : decimal(argc_text);
		next = msh_parse_line(input, argument_buffer, &argc, arguments);
		if (expected_argc < 0) {
			TEST_ASSERT(next == NULL);
		} else {
			TEST_ASSERT(next != NULL);
			TEST_EQ(expected_argc, argc);
			for (int index = 0; index < expected_argc; index++)
				TEST_STREQ(next_field(&cursor), arguments[index]);
			if (*cursor)
				TEST_STREQ(cursor, next);
		}
		line = line_end;
	}
}
