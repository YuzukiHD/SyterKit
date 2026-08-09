/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>

#include <initcall.h>

extern initcall_t const __initcall_start[];
extern initcall_t const __initcall_end[];

int do_initcall_range(const initcall_t *start, const initcall_t *end) {
	const initcall_t *entry;
	int first_error = 0;

	for (entry = start; entry < end; entry++) {
		int result;

		if (*entry == 0)
			continue;
		result = (*entry)();
		if (result != 0 && first_error == 0)
			first_error = result;
	}

	return first_error;
}

int do_initcalls(void) {
	static bool complete;
	static int result;

	if (complete)
		return result;

	complete = true;
	result = do_initcall_range(__initcall_start, __initcall_end);
	return result;
}
