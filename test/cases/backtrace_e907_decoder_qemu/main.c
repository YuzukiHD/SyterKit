/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdint.h>

int backtrace_test_riscv_call_size(uint32_t ins32, uint16_t ins16);
int backtrace_test_riscv_push_lr(uint32_t inst, int *offset);
int backtrace_test_riscv_stack_push(uint32_t inst);
int backtrace_test_riscv_return(uint32_t inst);
void qemu_puts(const char *text);
void qemu_exit(int success);

static int check(const char *name, int actual, int expected) {
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
	int offset = -1;
	int passed = 1;

	qemu_puts("TEST START backtrace_e907_decoder_qemu\n");
	passed &= check("jal", backtrace_test_riscv_call_size(0x000000efU, 0), 4);
	passed &= check("jalr", backtrace_test_riscv_call_size(0x000280e7U, 0), 4);
	passed &= check("c.jal", backtrace_test_riscv_call_size(0, 0x2001U), 2);
	passed &= check("c.jalr", backtrace_test_riscv_call_size(0, 0x9282U), 2);
	passed &= check("non-call", backtrace_test_riscv_call_size(0x00008067U, 0x8082U), 0);
	passed &= check("sw-ra-scan", backtrace_test_riscv_push_lr(0x00112623U, &offset), -1);
	passed &= check("sw-ra-offset", offset, 3);
	passed &= check("addi-sp", backtrace_test_riscv_stack_push(0xfe010113U), 8);
	passed &= check("ret", backtrace_test_riscv_return(0x00008067U), 0);
	if (passed) {
		qemu_puts("TEST PASS backtrace_e907_decoder_qemu\n");
		qemu_exit(1);
	}
	qemu_puts("TEST FAIL backtrace_e907_decoder_qemu\n");
	qemu_exit(0);
}
