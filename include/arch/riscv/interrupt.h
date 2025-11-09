/**
 * @file interrupt.h
 * @brief Interrupt control functions for RISC-V architecture.
 *
 * This header file provides functions for enabling and disabling interrupts
 * on RISC-V architecture.
 */
/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __INTERRUPT_H__
#define __INTERRUPT_H__

#include "csr.h"

/**
 * @brief Enable interrupts in RISC-V mode.
 *
 * This function enables machine interrupts by setting the MIE bit in the
 * Machine Status Register (mstatus).
 */
static inline void riscv_interrupt_enable(void) {
	/* Set MIE bit in mstatus register */
	csr_set(mstatus, MSTATUS_MIE);
}

/**
 * @brief Disable interrupts in RISC-V mode.
 *
 * This function disables machine interrupts by clearing the MIE bit in the
 * Machine Status Register (mstatus).
 */
static inline void riscv_interrupt_disable(void) {
	/* Clear MIE bit in mstatus register */
	csr_clear(mstatus, MSTATUS_MIE);
}

#endif /* __INTERRUPT_H__ */