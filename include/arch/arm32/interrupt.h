/**
 * @file interrupt.h
 * @brief Interrupt control functions for ARM32 architecture.
 *
 * This header file provides functions for enabling and disabling interrupts
 * on ARM32 architecture.
 */
/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __INTERRUPT_H__
#define __INTERRUPT_H__

/**
 * @brief Enable interrupts in ARM32 mode.
 *
 * This function enables IRQ interrupts by clearing the I-bit in the Current
 * Program Status Register (CPSR).
 */
static inline void arm32_interrupt_enable(void) {
	uint32_t tmp;

	__asm__ __volatile__("mrs %0, cpsr\n"
						 "bic %0, %0, #(1<<7)\n"
						 "msr cpsr_cxsf, %0"
						 : "=r"(tmp)
						 :
						 : "memory");
}

/**
 * @brief Disable interrupts in ARM32 mode.
 *
 * This function disables IRQ interrupts by setting the I-bit in the Current
 * Program Status Register (CPSR).
 */
static inline void arm32_interrupt_disable(void) {
	uint32_t tmp;

	__asm__ __volatile__("mrs %0, cpsr\n"
						 "orr %0, %0, #(1<<7)\n"
						 "msr cpsr_cxsf, %0"
						 : "=r"(tmp)
						 :
						 : "memory");
}

#endif /* __INTERRUPT_H__ */