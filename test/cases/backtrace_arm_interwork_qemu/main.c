/* SPDX-License-Identifier: GPL-2.0+ */

int dump_stack(void);
void qemu_puts(const char *text);
void qemu_exit(int success);

static int __attribute__((noinline, target("thumb"))) interwork_thumb_leaf(void)
{
	return dump_stack();
}

static int __attribute__((noinline, target("arm"))) interwork_arm_bridge(void)
{
	return interwork_thumb_leaf();
}

static int __attribute__((noinline, target("thumb"))) interwork_thumb_entry(void)
{
	return interwork_arm_bridge();
}

void test_boot(void)
{
	int levels;

	qemu_puts("TEST START backtrace_arm_interwork_qemu\n");
	levels = interwork_thumb_entry();
	if (levels >= 4) {
		qemu_puts("TEST PASS backtrace_arm_interwork_qemu\n");
		qemu_exit(1);
	}
	qemu_puts("TEST FAIL backtrace_arm_interwork_qemu\n");
	qemu_exit(0);
}
