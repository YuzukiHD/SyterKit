/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __RISCV_ASM_TRAP_H__
#define __RISCV_ASM_TRAP_H__

#include <linkage.h>

#define PT_REGS_SIZE (40 * REGSZ)

.macro SAVE_ALL
	csrw mscratch, sp
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
	csrrw t0, mscratch, zero
	csrr s0, mstatus
	csrr t1, mepc
	csrr t2, mtval
	csrr t3, mcause
	SREG t0, 2 * REGSZ(sp)
	SREG s0, 32 * REGSZ(sp)
	SREG t1, 33 * REGSZ(sp)
	SREG t2, 34 * REGSZ(sp)
	SREG t3, 35 * REGSZ(sp)
	li t0, -1
	SREG t0, 36 * REGSZ(sp)
	mv a0, sp
.endm

.macro RESTORE_ALL
	csrr a0, mscratch
	LREG t0, 32 * REGSZ(a0)
	csrw mstatus, t0
	LREG t0, 33 * REGSZ(a0)
	csrw mepc, t0
	LREG x1, 1 * REGSZ(a0)
	LREG x2, 2 * REGSZ(a0)
	LREG x3, 3 * REGSZ(a0)
	LREG x4, 4 * REGSZ(a0)
	LREG x5, 5 * REGSZ(a0)
	LREG x6, 6 * REGSZ(a0)
	LREG x7, 7 * REGSZ(a0)
	LREG x8, 8 * REGSZ(a0)
	LREG x9, 9 * REGSZ(a0)
	LREG x11, 11 * REGSZ(a0)
	LREG x12, 12 * REGSZ(a0)
	LREG x13, 13 * REGSZ(a0)
	LREG x14, 14 * REGSZ(a0)
	LREG x15, 15 * REGSZ(a0)
	LREG x16, 16 * REGSZ(a0)
	LREG x17, 17 * REGSZ(a0)
	LREG x18, 18 * REGSZ(a0)
	LREG x19, 19 * REGSZ(a0)
	LREG x20, 20 * REGSZ(a0)
	LREG x21, 21 * REGSZ(a0)
	LREG x22, 22 * REGSZ(a0)
	LREG x23, 23 * REGSZ(a0)
	LREG x24, 24 * REGSZ(a0)
	LREG x25, 25 * REGSZ(a0)
	LREG x26, 26 * REGSZ(a0)
	LREG x27, 27 * REGSZ(a0)
	LREG x28, 28 * REGSZ(a0)
	LREG x29, 29 * REGSZ(a0)
	LREG x30, 30 * REGSZ(a0)
	LREG x31, 31 * REGSZ(a0)
	csrw mscratch, zero
	LREG x10, 10 * REGSZ(a0)
	mret
.endm

#endif /* __RISCV_ASM_TRAP_H__ */
