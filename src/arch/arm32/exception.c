/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <types.h>

#include <mmu.h>
#include <timer.h>

#include <log.h>

extern void abort();

// #define START_UP_DEBUG
#ifdef START_UP_DEBUG
void int_to_hex_string(int value, char *buffer) {
	char hex_digits[] = "0123456789ABCDEF";
	int i;

	for (i = 0; i < 8; i++) {
		buffer[7 - i] = hex_digits[value & 0xF];
		value >>= 4;
	}
	buffer[8] = '\0';
}
#endif

static void show_regs(struct arm_regs_t *regs) {
	int i = 0;

	printk_error("pc : [<0x%08lx>] lr : [<0x%08lx>] cpsr: 0x%08lx\n", regs->pc, regs->lr, regs->cpsr);
	printk_error("sp : 0x%08lx esp : 0x%08lx\n", regs->sp, regs->esp);
	for (i = 12; i >= 0; i--) printk_error("r%-2d: 0x%08lx\n", i, regs->r[i]);
	printk_error("\n");

	char *PC = (char *) regs->pc;
	long *SP = (long *) regs->sp;
	char *LR = (char *) regs->lr;
	if (regs->cpsr & 0x20) {
		MAKE_THUMB_ADDR(PC);
	}
	backtrace(PC, SP, LR);
}

void __attribute__((weak)) arm32_do_undefined_instruction(struct arm_regs_t *regs) {
	printk_error("undefined_instruction\n");
	show_regs(regs);
	regs->pc += 4;
	abort();
}

void __attribute__((weak)) arm32_do_software_interrupt(struct arm_regs_t *regs) {
	printk_error("software_interrupt\n");
	show_regs(regs);
	regs->pc += 4;
	abort();
}

void __attribute__((weak)) arm32_do_prefetch_abort(struct arm_regs_t *regs) {
	printk_error("prefetch_abort\n");
	show_regs(regs);
	regs->pc += 4;
	abort();
}

void __attribute__((weak)) arm32_do_data_abort(struct arm_regs_t *regs) {
	printk_error("data_abort\n");
	show_regs(regs);
	regs->pc += 4;
	abort();
}

void __attribute__((weak)) arm32_do_irq(struct arm_regs_t *regs) {
	printk_error("undefined IRQ\n");
	show_regs(regs);
	abort();
}

void __attribute__((weak)) arm32_do_fiq(struct arm_regs_t *regs) {
	printk_error("undefined FIQ\n");
	show_regs(regs);
	abort();
}
