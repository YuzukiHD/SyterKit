/* SPDX-License-Identifier: GPL-2.0+ */

int dump_stack(void);
void qemu_puts(const char *text);
void qemu_exit(int success);

static volatile int optimizer_guard = 1;

static int __attribute__((noinline)) arm_optimized_level_three(int value)
{
	return dump_stack() + value + optimizer_guard;
}

static int __attribute__((noinline)) arm_optimized_level_two(int value)
{
	return arm_optimized_level_three(value + optimizer_guard);
}

static int __attribute__((noinline)) arm_optimized_level_one(void)
{
	return arm_optimized_level_two(optimizer_guard);
}

void test_boot(void)
{
	int result;

	qemu_puts("TEST START backtrace_arm_optimized_qemu\n");
	result = arm_optimized_level_one();
	if (result >= 7) {
		qemu_puts("TEST PASS backtrace_arm_optimized_qemu\n");
		qemu_exit(1);
	}
	qemu_puts("TEST FAIL backtrace_arm_optimized_qemu\n");
	qemu_exit(0);
}
