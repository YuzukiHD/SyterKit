/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __DRIVERS_INTC_CLIC_H__
#define __DRIVERS_INTC_CLIC_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <drivers/intc/intc.h>

#define SUNXI_CLIC_COMPATIBLE "thead,c900-clic"
#define SUNXI_CLIC_MAX_IRQS 256U

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum irq_trigger_type { IRQ_TRIGGER_TYPE_LEVEL, IRQ_TRIGGER_TYPE_EDGE_RISING, IRQ_TRIGGER_TYPE_EDGE_FALLING, IRQ_TRIGGER_TYPE_EDGE_BOTH } irq_trigger_type_t;

typedef struct sunxi_clic {
	int dt_node;
	uintptr_t base;
	size_t size;
	uint32_t irq_count;
	bool initialized;
	irq_handler_t handlers[SUNXI_CLIC_MAX_IRQS];
} sunxi_clic_t;

/**
 * @brief Handles the IRQ
 * 
 */
void do_irq(uint64_t cause);

/**
 * @brief Initializes a CLIC instance.
 *
 * @param clic CLIC instance populated from the static devicetree.
 * @return 0 on success, or an error code.
 */
int sunxi_clic_init(sunxi_clic_t *clic);

/**
 * @brief Shuts down a CLIC instance.
 *
 * @param clic Initialized CLIC instance.
 * @return 0 on success, or an error code.
 */
int sunxi_clic_exit(sunxi_clic_t *clic);

/**
 * @brief Initializes the interrupt mechanism
 * 
 * @return 0 on success, or an error code
 */
int arch_interrupt_init(void);

/** @brief Load the CLIC configuration from DTS and enable interrupts. */
int sunxi_clic_startup(void);

/**
 * @brief Exits the interrupt mechanism
 * 
 * @return 0 on success, or an error code
 */
int arch_interrupt_exit(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __DRIVERS_INTC_CLIC_H__ */
