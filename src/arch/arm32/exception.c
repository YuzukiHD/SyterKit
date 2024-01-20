/* SPDX-License-Identifier: Apache-2.0 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <mmu.h>
#include <timer.h>

#include <log.h>

struct arm_regs_t {
    uint32_t esp;
    uint32_t cpsr;
    uint32_t r[13];
    uint32_t sp;
    uint32_t lr;
    uint32_t pc;
};

static void show_regs(struct arm_regs_t *regs) {
    int i = 0;

    printk(LOG_LEVEL_ERROR, "pc : [<0x%08lx>] lr : [<0x%08lx>] cpsr: 0x%08lx\n", regs->pc, regs->lr, regs->cpsr);
    printk(LOG_LEVEL_ERROR, "sp : 0x%08lx esp : 0x%08lx\n", regs->sp, regs->esp);
    for (i = 12; i >= 0; i--)
        printk(LOG_LEVEL_ERROR, "r%-2d: 0x%08lx\n", i, regs->r[i]);
    printk(LOG_LEVEL_ERROR, "\n");

    mdelay(1000);
}

void arm32_do_undefined_instruction(struct arm_regs_t *regs) {
    printk(LOG_LEVEL_ERROR, "undefined_instruction\n");
    show_regs(regs);
}

void arm32_do_software_interrupt(struct arm_regs_t *regs) {
    printk(LOG_LEVEL_ERROR, "software_interrupt\n");
    show_regs(regs);
}

void arm32_do_prefetch_abort(struct arm_regs_t *regs) {
    printk(LOG_LEVEL_ERROR, "prefetch_abort\n");
    show_regs(regs);
}

void arm32_do_data_abort(struct arm_regs_t *regs) {
    printk(LOG_LEVEL_ERROR, "data_abort\n");
    show_regs(regs);
}

void arm32_do_irq(struct arm_regs_t *regs) {
    printk(LOG_LEVEL_ERROR, "undefined IRQ\n");
    show_regs(regs);
}

void arm32_do_fiq(struct arm_regs_t *regs) {
    printk(LOG_LEVEL_ERROR, "undefined FIQ\n");
    show_regs(regs);
}
