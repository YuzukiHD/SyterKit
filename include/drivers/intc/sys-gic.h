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

/**
 * @brief Structure for IRQ handler
 * 
 * @param data Pointer to data associated with the IRQ handler
 * @param func Function pointer to the IRQ handler function
 */
typedef struct _irq_handler {
    void *data;
    void (*func)(void *data);
} irq_handler_t;

/**
 * @brief Type definition for interrupt handler function
 * 
 * @param Pointer to data associated with the interrupt handler
 */
typedef void(interrupt_handler_t)(void *);

/**
 * @brief Installs a handler for the specified IRQ
 * 
 * @param irq IRQ number
 * @param handle_irq Function pointer to the interrupt handler function
 * @param data Pointer to data associated with the interrupt handler
 */
void irq_install_handler(int irq, interrupt_handler_t handle_irq, void *data);

/**
 * @brief Handles the IRQ with ARM registers
 * 
 * @param regs Pointer to the ARM registers
 */
void do_irq(struct arm_regs_t *regs);

/**
 * @brief Initializes the interrupt mechanism
 * 
 * @return 0 on success, or an error code
 */
int arch_interrupt_init(void);

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

/**
 * @brief Frees the resources associated with the specified IRQ handler
 * 
 * @param irq IRQ number
 */
void irq_free_handler(int irq);

/**
 * @brief Enables the specified IRQ
 * 
 * @param irq_no IRQ number to be enabled
 * @return 0 on success, or an error code
 */
int irq_enable(int irq_no);

/**
 * @brief Disables the specified IRQ
 * 
 * @param irq_no IRQ number to be disabled
 * @return 0 on success, or an error code
 */
int irq_disable(int irq_no);

#ifdef __cplusplus
}
#endif// __cplusplus

#endif// _SYS_GIC_H_