/* SPDX-License-Identifier: Apache-2.0 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <mmu.h>
#include <timer.h>

#include <log.h>

#include <sys-gic.h>

static void show_regs(struct arm_regs_t *regs) {
    int i = 0;

    printk_error("pc : [<0x%08lx>] lr : [<0x%08lx>] cpsr: 0x%08lx\n", regs->pc, regs->lr, regs->cpsr);
    printk_error("sp : 0x%08lx esp : 0x%08lx\n", regs->sp, regs->esp);
    for (i = 12; i >= 0; i--)
        printk_error("r%-2d: 0x%08lx\n", i, regs->r[i]);
    printk_error("\n");
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
