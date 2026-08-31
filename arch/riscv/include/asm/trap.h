/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file trap.h
 * @brief RISC-V assembly register-save and restore macros.
 *
 * SAVE_ALL builds the trap frame consumed by the C exception handler;
 * RESTORE_ALL restores machine state and returns with @c mret. The macros
 * use REGSZ so the same layout works for RV32 and RV64 builds.
 */

#ifndef __RISCV_ASM_TRAP_H__
#define __RISCV_ASM_TRAP_H__

#include <linkage.h>

#define PT_REGS_SIZE (40 * REGSZ)

.macro SAVE_ALL
	addi sp, sp, -PT_REGS_SIZE
	SREG zero, 0 * REGSZ(sp)
	SREG x1, 1 * REGSZ(sp)
	SREG x3, 3 * REGSZ(sp)
	SREG x4, 4 * REGSZ(sp)
	SREG x5, 5 * REGSZ(sp)
	SREG x6, 6 * REGSZ(sp)
	SREG x7, 7 * REGSZ(sp)
	SREG x8, 8 * REGSZ(sp)
	SREG x9, 9 * REGSZ(sp)
	SREG x10, 10 * REGSZ(sp)
	SREG x11, 11 * REGSZ(sp)
	SREG x12, 12 * REGSZ(sp)
	SREG x13, 13 * REGSZ(sp)
	SREG x14, 14 * REGSZ(sp)
	SREG x15, 15 * REGSZ(sp)
	SREG x16, 16 * REGSZ(sp)
	SREG x17, 17 * REGSZ(sp)
	SREG x18, 18 * REGSZ(sp)
	SREG x19, 19 * REGSZ(sp)
	SREG x20, 20 * REGSZ(sp)
	SREG x21, 21 * REGSZ(sp)
	SREG x22, 22 * REGSZ(sp)
	SREG x23, 23 * REGSZ(sp)
	SREG x24, 24 * REGSZ(sp)
	SREG x25, 25 * REGSZ(sp)
	SREG x26, 26 * REGSZ(sp)
	SREG x27, 27 * REGSZ(sp)
	SREG x28, 28 * REGSZ(sp)
	SREG x29, 29 * REGSZ(sp)
	SREG x30, 30 * REGSZ(sp)
	SREG x31, 31 * REGSZ(sp)
	addi t0, sp, PT_REGS_SIZE
	SREG t0, 2 * REGSZ(sp)
	csrr t0, mstatus
	SREG t0, 32 * REGSZ(sp)
	csrr t0, mepc
	SREG t0, 33 * REGSZ(sp)
	csrr t0, mtval
	SREG t0, 34 * REGSZ(sp)
	csrr t0, mcause
	SREG t0, 35 * REGSZ(sp)
	li t0, -1
	SREG t0, 36 * REGSZ(sp)
	mv a0, sp
.endm

.macro RESTORE_ALL
	LREG t0, 32 * REGSZ(sp)
	csrw mstatus, t0
	LREG t0, 33 * REGSZ(sp)
	csrw mepc, t0
	LREG x1, 1 * REGSZ(sp)
	LREG x3, 3 * REGSZ(sp)
	LREG x4, 4 * REGSZ(sp)
	LREG x5, 5 * REGSZ(sp)
	LREG x6, 6 * REGSZ(sp)
	LREG x7, 7 * REGSZ(sp)
	LREG x8, 8 * REGSZ(sp)
	LREG x9, 9 * REGSZ(sp)
	LREG x10, 10 * REGSZ(sp)
	LREG x11, 11 * REGSZ(sp)
	LREG x12, 12 * REGSZ(sp)
	LREG x13, 13 * REGSZ(sp)
	LREG x14, 14 * REGSZ(sp)
	LREG x15, 15 * REGSZ(sp)
	LREG x16, 16 * REGSZ(sp)
	LREG x17, 17 * REGSZ(sp)
	LREG x18, 18 * REGSZ(sp)
	LREG x19, 19 * REGSZ(sp)
	LREG x20, 20 * REGSZ(sp)
	LREG x21, 21 * REGSZ(sp)
	LREG x22, 22 * REGSZ(sp)
	LREG x23, 23 * REGSZ(sp)
	LREG x24, 24 * REGSZ(sp)
	LREG x25, 25 * REGSZ(sp)
	LREG x26, 26 * REGSZ(sp)
	LREG x27, 27 * REGSZ(sp)
	LREG x28, 28 * REGSZ(sp)
	LREG x29, 29 * REGSZ(sp)
	LREG x30, 30 * REGSZ(sp)
	LREG x31, 31 * REGSZ(sp)
	addi sp, sp, PT_REGS_SIZE
	mret
.endm

#endif /* __RISCV_ASM_TRAP_H__ */
