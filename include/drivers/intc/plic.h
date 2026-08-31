/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __DRIVERS_INTC_PLIC_H__
#define __DRIVERS_INTC_PLIC_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <drivers/intc/intc.h>

#define SUNXI_PLIC_COMPATIBLE "allwinner,sunxi-plic"
#define SUNXI_PLIC_MAX_IRQS 256U

typedef struct sunxi_plic {
	int dt_node;
	uintptr_t base;
	size_t size;
	uint32_t irq_count;
	bool initialized;
	irq_handler_t handlers[SUNXI_PLIC_MAX_IRQS];
} sunxi_plic_t;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

int sunxi_plic_init(sunxi_plic_t *plic);
int sunxi_plic_exit(sunxi_plic_t *plic);
int arch_interrupt_init(void);
int arch_interrupt_exit(void);
int sunxi_plic_startup(void);
void sunxi_plic_handle_irq(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __DRIVERS_INTC_PLIC_H__ */
