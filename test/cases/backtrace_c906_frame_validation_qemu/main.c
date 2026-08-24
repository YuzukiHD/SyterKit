/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdint.h>

#include <backtrace.h>

void qemu_puts(const char *text);
void qemu_exit(int success);
extern unsigned char __stack_srv_end[];

static void __attribute__((noinline)) validation_start_anchor(void)
{
	asm volatile("" ::: "memory");
}

static void __attribute__((noinline)) validation_frame_one(void)
{
	asm volatile("" ::: "memory");
}

static void __attribute__((noinline)) validation_frame_two(void)
{
	asm volatile("" ::: "memory");
}

void test_boot(void)
{
	uintptr_t words[4] __attribute__((aligned(16)));
	struct backtrace_context context;
	int valid_chain;
	int misaligned;
	int outside_stack;

	words[0] = (uintptr_t)&words[4];
	words[1] = (uintptr_t)validation_frame_one;
	words[2] = 0U;
	words[3] = (uintptr_t)validation_frame_two;
	context.pc = (uintptr_t)validation_start_anchor;
	context.sp = (uintptr_t)&words[0];
	context.fp = (uintptr_t)&words[2];
	context.lr = 0U;

	qemu_puts("TEST START backtrace_c906_frame_validation_qemu\n");
	valid_chain = backtrace_from_context(&context);
	context.fp = (uintptr_t)&words[1];
	misaligned = backtrace_from_context(&context);
	context.fp = (uintptr_t)__stack_srv_end + 16U;
	outside_stack = backtrace_from_context(&context);
	if (valid_chain == 3 && misaligned == 1 && outside_stack == 1) {
		qemu_puts("CHECK valid-chain PASS\n");
		qemu_puts("CHECK misaligned-fp PASS\n");
		qemu_puts("CHECK outside-stack PASS\n");
		qemu_puts("TEST PASS backtrace_c906_frame_validation_qemu\n");
		qemu_exit(1);
	}

	qemu_puts("TEST FAIL backtrace_c906_frame_validation_qemu\n");
	qemu_exit(0);
}
