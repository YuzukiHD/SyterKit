/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __DRIVERS_INTC_GIC_H__
#define __DRIVERS_INTC_GIC_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <common.h>

#include <drivers/intc/intc.h>

#define SUNXI_GIC_COMPATIBLE "arm,gic-400"
#define SUNXI_GIC_MAX_IRQS 256U

typedef struct sunxi_gic {
	int dt_node;
	uintptr_t distributor_base;
	size_t distributor_size;
	uintptr_t cpu_interface_base;
	size_t cpu_interface_size;
	uint32_t irq_count;
	bool initialized;
} sunxi_gic_t;

struct arm_regs_t;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief Handles the IRQ with ARM registers
 * 
 * @param regs Pointer to the ARM registers
 */
void do_irq(struct arm_regs_t *regs);

/**
 * @brief Initializes a GIC instance.
 *
 * @param gic GIC instance populated from the static devicetree.
 * @return 0 on success, or an error code.
 */
int sunxi_gic_init(sunxi_gic_t *gic);

/**
 * @brief Shuts down a GIC instance.
 *
 * @param gic Initialized GIC instance.
 * @return 0 on success, or an error code.
 */
int sunxi_gic_exit(sunxi_gic_t *gic);

/**
 * @brief Initializes the interrupt mechanism
 * 
 * @return 0 on success, or an error code
 */
int arch_interrupt_init(void);

/** @brief Load the GIC configuration from DTS and enable interrupts. */
int sunxi_gic_startup(void);

/**
 * @brief Exits the interrupt mechanism
 * 
 * @return 0 on success, or an error code
 */
int arch_interrupt_exit(void);

/**
 * @brief Initializes the Sunxi GIC CPU interface
 * 
 * @param cpu CPU identifier
 * @return 0 on success, or an error code
 */
int sunxi_gic_cpu_interface_init(int cpu);

/**
 * @brief Exits the Sunxi GIC CPU interface
 * 
 * @return 0 on success, or an error code
 */
int sunxi_gic_cpu_interface_exit(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __DRIVERS_INTC_GIC_H__ */
