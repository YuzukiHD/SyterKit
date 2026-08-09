/* SPDX-License-Identifier: GPL-2.0+ */

#include <os.h>

#include "syter_test.h"

struct callback_state {
	unsigned int calls;
	unsigned int last_event;
};

static void count_callback(void *arg, uint32_t event) {
	struct callback_state *state = arg;

	state->calls++;
	state->last_event = event;
}

static unsigned int parse_value(char **cursor) {
	unsigned int value = 0;

	while (**cursor == ' ')
		(*cursor)++;
	while (**cursor >= '0' && **cursor <= '9')
		value = value * 10U + (unsigned int) (*(*cursor)++ - '0');
	return value;
}

void test_case_main(const char *case_dir) {
	struct callback_state finite = {0};
	struct callback_state forever = {0};
	timer_t finite_timer;
	timer_t forever_timer;
	char data[TEST_DATA_MAX];
	char *cursor;
	unsigned int finite_runs;
	unsigned int finite_interval;
	unsigned int forever_runs;
	unsigned int forever_interval;
	int length;

	length = test_load_data(case_dir, "data/schedule.txt", data, sizeof(data));
	TEST_ASSERT(length > 0);
	if (length <= 0)
		return;

	cursor = data;
	finite_runs = parse_value(&cursor);
	finite_interval = parse_value(&cursor);
	while (*cursor && *cursor != '\n')
		cursor++;
	if (*cursor)
		cursor++;
	forever_runs = parse_value(&cursor);
	forever_interval = parse_value(&cursor);

	timer_create(&finite_timer, count_callback, &finite);
	timer_create(&forever_timer, count_callback, &forever);
	timer_start(&finite_timer, finite_runs, finite_interval);
	timer_start(&forever_timer, forever_runs, forever_interval);

	for (unsigned int tick = 0; tick < 12; tick++)
		timer_handle();

	TEST_EQ(finite_runs, finite.calls);
	TEST_EQ(finite_runs - 1U, finite.last_event);
	TEST_EQ(12U / forever_interval, forever.calls);
	TEST_EQ(forever.calls - 1U, forever.last_event);
}
