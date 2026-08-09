/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdint.h>

uint32_t backtrace_test_thumb_expand_imm(uint32_t inst);
int backtrace_test_arm_stack_push(uint32_t inst);
int backtrace_test_thumb_stack_push(uint32_t inst, int thumb32bit);
int backtrace_test_arm_return(uint32_t inst);
int backtrace_test_thumb_return(uint32_t inst, int thumb32bit);
void qemu_puts(const char *text);
void qemu_exit(int success);

static int check(const char *name, uint32_t actual, uint32_t expected) {
	if (actual != expected) {
		qemu_puts("CHECK FAIL ");
		qemu_puts(name);
		qemu_puts("\n");
		return 0;
	}
	qemu_puts("CHECK ");
	qemu_puts(name);
	qemu_puts(" PASS\n");
	return 1;
}

void test_boot(void) {
	int passed = 1;

	qemu_puts("TEST START backtrace_arm_decoder_qemu\n");
	passed &= check("thumb-imm8", backtrace_test_thumb_expand_imm(0xf1ad0dffU), 255U);
	passed &= check("thumb-rotate", backtrace_test_thumb_expand_imm(0xf5ad7d80U), 256U);
	passed &= check("thumb-replicate", backtrace_test_thumb_expand_imm(0xf1ad1dffU), 0x00ff00ffU);
	passed &= check("arm-sub", (uint32_t) backtrace_test_arm_stack_push(0xe24ddc01U), 64U);
	passed &= check("thumb-sub", (uint32_t) backtrace_test_thumb_stack_push(0x0000b084U, 0), 4U);
	passed &= check("thumb-sub-wide", (uint32_t) backtrace_test_thumb_stack_push(0xf5ad7d80U, 1), 64U);
	passed &= check("arm-return", (uint32_t) backtrace_test_arm_return(0xe12fff1eU), 0U);
	passed &= check("thumb-return", (uint32_t) backtrace_test_thumb_return(0x00004770U, 0), 0U);
	if (passed) {
		qemu_puts("TEST PASS backtrace_arm_decoder_qemu\n");
		qemu_exit(1);
	}
	qemu_puts("TEST FAIL backtrace_arm_decoder_qemu\n");
	qemu_exit(0);
}
