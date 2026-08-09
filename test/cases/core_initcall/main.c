/* SPDX-License-Identifier: GPL-2.0+ */

#include <initcall.h>

#include "syter_test.h"

static unsigned int call_order[6];
static unsigned int call_count;

static int record_call(unsigned int level, int result) {
	call_order[call_count++] = level;
	return result;
}

static int test_early_init(void) {
	return record_call(1U, 0);
}
early_initcall(test_early_init);

static int test_core_init(void) {
	return record_call(2U, -7);
}
core_initcall(test_core_init);

static int test_repeated_init(void) {
	return record_call(5U, 0);
}
core_initcall(test_repeated_init);
core_initcall(test_repeated_init);

static int test_device_init(void) {
	return record_call(3U, 0);
}
device_initcall(test_device_init);

static int test_late_init(void) {
	return record_call(4U, -9);
}
late_initcall(test_late_init);

void test_case_main(const char *case_dir) {
	static const unsigned int expected_order[] = {1U, 2U, 5U, 5U, 3U, 4U};

	(void) case_dir;

	TEST_EQ(-7, do_initcalls());
	TEST_EQ(6U, call_count);
	for (unsigned int index = 0; index < call_count; index++)
		TEST_EQ(expected_order[index], call_order[index]);

	TEST_EQ(-7, do_initcalls());
	TEST_EQ(6U, call_count);
}
