/* SPDX-License-Identifier: GPL-2.0+ */

#include <stddef.h>
#include <stdint.h>

#include <backtrace.h>

void qemu_puts(const char *text);
void qemu_exit(int success);
extern unsigned char __stack_srv_start[];

static void __attribute__((noinline)) valid_pc_anchor(void) {
	asm volatile("" ::: "memory");
}

void test_boot(void) {
	struct backtrace_context context = {
		.pc = 4U,
		.sp = (uintptr_t) __stack_srv_start,
		.fp = 0U,
		.lr = 8U,
	};
	int bad_pc;
	int bad_sp;
	int null_context;

	qemu_puts("TEST START backtrace_c906_invalid_context_qemu\n");
	bad_pc = backtrace_from_context(&context);
	context.pc = (uintptr_t) valid_pc_anchor;
	context.sp = 4U;
	context.lr = (uintptr_t) valid_pc_anchor;
	bad_sp = backtrace_from_context(&context);
	null_context = backtrace_from_context(NULL);
	if (bad_pc == 0 && bad_sp == 0 && null_context == 0) {
		qemu_puts("CHECK bad-pc PASS\n");
		qemu_puts("CHECK bad-sp PASS\n");
		qemu_puts("CHECK null-context PASS\n");
		qemu_puts("TEST PASS backtrace_c906_invalid_context_qemu\n");
		qemu_exit(1);
	}

	qemu_puts("TEST FAIL backtrace_c906_invalid_context_qemu\n");
	qemu_exit(0);
}
