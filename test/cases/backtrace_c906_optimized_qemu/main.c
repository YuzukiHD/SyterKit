/* SPDX-License-Identifier: GPL-2.0+ */

#include <backtrace.h>

void qemu_puts(const char *text);
void qemu_exit(int success);
static volatile int optimizer_guard = 1;

static int __attribute__((noinline)) optimized_c906_level_three(int value) {
	return dump_stack() + value + optimizer_guard;
}

static int __attribute__((noinline)) optimized_c906_level_two(int value) {
	return optimized_c906_level_three(value + optimizer_guard);
}

static int __attribute__((noinline)) optimized_c906_level_one(void) {
	return optimized_c906_level_two(1);
}

void test_boot(void) {
	int result;

	qemu_puts("TEST START backtrace_c906_optimized_qemu\n");
	result = optimized_c906_level_one();
	if (result >= 7) {
		qemu_puts("TEST PASS backtrace_c906_optimized_qemu\n");
		qemu_exit(1);
	}

	qemu_puts("TEST FAIL backtrace_c906_optimized_qemu\n");
	qemu_exit(0);
}
