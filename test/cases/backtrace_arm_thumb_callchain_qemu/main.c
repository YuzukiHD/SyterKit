/* SPDX-License-Identifier: GPL-2.0+ */

int dump_stack(void);
void qemu_puts(const char *text);
void qemu_exit(int success);

static int __attribute__((noinline)) thumb_level_three(void)
{
	return dump_stack();
}

static int __attribute__((noinline)) thumb_level_two(void)
{
	return thumb_level_three();
}

static int __attribute__((noinline)) thumb_level_one(void)
{
	return thumb_level_two();
}

void test_boot(void)
{
	int levels;

	qemu_puts("TEST START backtrace_arm_thumb_callchain_qemu\n");
	levels = thumb_level_one();
	if (levels >= 4) {
		qemu_puts("TEST PASS backtrace_arm_thumb_callchain_qemu\n");
		qemu_exit(1);
	}

	qemu_puts("TEST FAIL backtrace_arm_thumb_callchain_qemu\n");
	qemu_exit(0);
}
