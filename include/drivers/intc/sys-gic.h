/* SPDX-License-Identifier: GPL-2.0+ */

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

#include <sys-intc.h>

#include <reg-ncat.h>
#include <reg-gic.h>

#ifdef __cplusplus
extern "C" {
#endif// __cplusplus

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

#ifdef __cplusplus
}
#endif// __cplusplus

#endif// _SYS_GIC_H_