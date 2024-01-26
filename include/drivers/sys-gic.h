/* SPDX-License-Identifier: Apache-2.0 */

#ifndef _SYS_GIC_H_
#define _SYS_GIC_H_

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <common.h>
#include <log.h>

#include <reg-ncat.h>

#include "reg-gic.h"

#ifdef __cplusplus
extern "C" {
#endif// __cplusplus

typedef struct _irq_handler {
    void *data;
    void (*func)(void *data);
} irq_handler_t;

typedef void(interrupt_handler_t)(void *);

void irq_install_handler(int irq, interrupt_handler_t handle_irq, void *data);

void do_irq(struct arm_regs_t *regs);

int arch_interrupt_init(void);

int arch_interrupt_exit(void);

int sunxi_gic_cpu_interface_init(int cpu);

int sunxi_gic_cpu_interface_exit(void);

void irq_free_handler(int irq);

int irq_enable(int irq_no);

int irq_disable(int irq_no);

#ifdef __cplusplus
}
#endif// __cplusplus

#endif// _SYS_GIC_H_