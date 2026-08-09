/* SPDX-License-Identifier: GPL-2.0+ */

#include <stddef.h>
#include <stdint.h>

#include <log.h>
#include <xformat.h>

#include "syter_test.h"

static char output[1024];
static size_t output_length;

static void reset_output(void) {
	output_length = 0;
	output[0] = '\0';
}

void uart_log_putchar(void *arg, char value) {
	(void) arg;
	if (output_length + 1U < sizeof(output))
		output[output_length++] = value;
	output[output_length] = '\0';
}

uint32_t time_us(void) {
	return 2234567U;
}

uint32_t get_init_timestamp(void) {
	return 1000000U;
}

void test_case_main(const char *case_dir) {
	char expected[TEST_DATA_MAX];
	char *separator;
	int length;

	length = test_load_data(case_dir, "data/output.txt", expected,
				sizeof(expected));
	TEST_ASSERT(length > 0);
	if (length <= 0)
		return;
	if (expected[length - 1] == '\n')
		expected[length - 1] = '\0';
	separator = expected;
	while (*separator && *separator != '\n')
		separator++;
	TEST_ASSERT(*separator == '\n');
	if (!*separator)
		return;
	*separator++ = '\0';

	reset_output();
	xformat(uart_log_putchar, NULL, "%+05d/%#x/%s", -7, 42U, "ok");
	TEST_STREQ(expected, output);

	reset_output();
	printk(LOG_LEVEL_INFO, "value=%d", 42);
	TEST_STREQ(separator, output);
}
