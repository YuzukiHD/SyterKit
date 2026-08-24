/* SPDX-License-Identifier: GPL-2.0+ */

#include <stddef.h>

#include <cli/cli.h>
#include <cli/cli_history.h>

#include "syter_test.h"

static unsigned char input[256];
static size_t input_length;
static size_t input_position;
static char terminal_output[1024];
static size_t terminal_length;

static int hex_digit(char value)
{
	if (value >= '0' && value <= '9')
		return value - '0';
	if (value >= 'a' && value <= 'f')
		return value - 'a' + 10;
	if (value >= 'A' && value <= 'F')
		return value - 'A' + 10;
	return -1;
}

int uart_putchar(int value)
{
	if (terminal_length + 1U < sizeof(terminal_output))
		terminal_output[terminal_length++] = (char)value;
	terminal_output[terminal_length] = '\0';
	return value;
}

int uart_puts(const char *text)
{
	int count = 0;

	while (*text) {
		uart_putchar(*text++);
		count++;
	}
	return count;
}

int uart_getchar(void)
{
	if (input_position >= input_length)
		return '\n';
	return input[input_position++];
}

void test_case_main(const char *case_dir)
{
	char hex_data[TEST_DATA_MAX];
	char expected[TEST_DATA_MAX];
	char line[MSH_CMDLINE_CHAR_MAX];
	int expected_length;
	int hex_length;

	hex_length = test_load_data(case_dir, "data/input.hex", hex_data, sizeof(hex_data));
	expected_length = test_load_data(case_dir, "data/result.txt", expected, sizeof(expected));
	TEST_ASSERT(hex_length > 0);
	TEST_ASSERT(expected_length > 0);
	if (hex_length <= 0 || expected_length <= 0)
		return;
	if (expected[expected_length - 1] == '\n')
		expected[expected_length - 1] = '\0';

	for (int index = 0; index + 1 < hex_length; index += 2) {
		int high;
		int low;

		if (hex_data[index] == '\n')
			break;
		high = hex_digit(hex_data[index]);
		low = hex_digit(hex_data[index + 1]);
		TEST_ASSERT(high >= 0 && low >= 0);
		if (high < 0 || low < 0)
			return;
		input[input_length++] = (unsigned char)((high << 4) | low);
	}

	TEST_EQ(expected_length - 1, msh_get_cmdline(line));
	TEST_STREQ(expected, line);
	TEST_EQ(1, get_history_count());
	TEST_STREQ(expected, history_get(0));
	TEST_ASSERT(terminal_length > 0);
}
