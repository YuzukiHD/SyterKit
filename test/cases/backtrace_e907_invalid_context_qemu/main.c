/* SPDX-License-Identifier: GPL-2.0+ */

int backtrace(char *pc, long *sp, char *lr);
void qemu_puts(const char *text);
void qemu_exit(int success);
extern unsigned char __stack_srv_start[];

static void __attribute__((noinline)) valid_pc_anchor(void)
{
	asm volatile("" ::: "memory");
}

void test_boot(void)
{
	long *valid_sp = (long *)__stack_srv_start;
	int bad_pc;
	int bad_sp;
	int bad_context;

	qemu_puts("TEST START backtrace_e907_invalid_context_qemu\n");
	bad_pc = backtrace((char *)4, valid_sp, (char *)8);
	bad_sp = backtrace((char *)valid_pc_anchor, (long *)4, (char *)valid_pc_anchor);
	bad_context = backtrace((char *)4, (long *)4, (char *)4);
	if (bad_pc == 0 && bad_sp == 0 && bad_context == 0) {
		qemu_puts("CHECK bad-pc PASS\n");
		qemu_puts("CHECK bad-sp PASS\n");
		qemu_puts("CHECK bad-context PASS\n");
		qemu_puts("TEST PASS backtrace_e907_invalid_context_qemu\n");
		qemu_exit(1);
	}

	qemu_puts("TEST FAIL backtrace_e907_invalid_context_qemu\n");
	qemu_exit(0);
}
