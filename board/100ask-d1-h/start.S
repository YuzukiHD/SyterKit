/* SPDX-License-Identifier: GPL-2.0+ */

#include <linkage.h>
#include <csr.h>

	.text
	.align 4
	.globl _start
_start:
	/* disable interrupt */
	csrw mie, zero

	/* enable theadisaee */
	li t1, (0x1 << 22) | (1 << 21) | (1 << 15)
	csrs mxstatus, t1

	/* invaild ICACHE/DCACHE/BTB/BHT */
	li t2, 0x30013
	csrs mcor, t2

	/* Config pmp register */
	li t0, (0xfffffffc >> 2)
	csrw pmpaddr0, t0
	li t0, ((0x0 << 7) | (0x1 << 3) | (0x1 << 2) | (0x1 << 1) | (0x1 << 0))
	csrw pmpcfg0, t0

	/* Mask all interrupts */
	csrw mideleg, zero
	csrw medeleg, zero
	csrw mie, zero
	csrw mip, zero

	/* Setup exception vectors */
	la t1, _image_start
	LREG t1, (t1)
	la t2, _start
	sub t0, t1, t2
	la a0, vectors
	add a0, a0, t0
	csrw mtvec, a0

	/* Enable fpu and accelerator and vector if present */
	li t0, MSTATUS_FS | MSTATUS_XS | (3 << 23)
	csrs mstatus, t0

	la sp, _stack_end

	jal clear_bss

	jal set_timer_count

	jal main

clear_bss:
	la	t0, _s_bss
	la	t1, _e_bss

clbss_1:
	sw zero, 0(t0)
	addi t0, t0, 4
	blt t0, t1, clbss_1
	ret

	/*
	* Exception vectors.
	*/
	.align 4
	.globl vectors
vectors:
	csrw mscratch, sp
	addi sp, sp, -(37 * REGSZ)
	SREG x1, 1 * REGSZ(x2)
	SREG x3, 3 * REGSZ(x2)
	SREG x4, 4 * REGSZ(x2)
	SREG x5, 5 * REGSZ(x2)
	SREG x6, 6 * REGSZ(x2)
	SREG x7, 7 * REGSZ(x2)
	SREG x8, 8 * REGSZ(x2)
	SREG x9, 9 * REGSZ(x2)
	SREG x10, 10 * REGSZ(x2)
	SREG x11, 11 * REGSZ(x2)
	SREG x12, 12 * REGSZ(x2)
	SREG x13, 13 * REGSZ(x2)
	SREG x14, 14 * REGSZ(x2)
	SREG x15, 15 * REGSZ(x2)
	SREG x16, 16 * REGSZ(x2)
	SREG x17, 17 * REGSZ(x2)
	SREG x18, 18 * REGSZ(x2)
	SREG x19, 19 * REGSZ(x2)
	SREG x20, 20 * REGSZ(x2)
	SREG x21, 21 * REGSZ(x2)
	SREG x22, 22 * REGSZ(x2)
	SREG x23, 23 * REGSZ(x2)
	SREG x24, 24 * REGSZ(x2)
	SREG x25, 25 * REGSZ(x2)
	SREG x26, 26 * REGSZ(x2)
	SREG x27, 27 * REGSZ(x2)
	SREG x28, 28 * REGSZ(x2)
	SREG x29, 29 * REGSZ(x2)
	SREG x30, 30 * REGSZ(x2)
	SREG x31, 31 * REGSZ(x2)
	csrrw t0, mscratch, x0
	csrr s0, mstatus
	csrr t1, mepc
#ifdef CONFIG_RISCV_COMPILER_GCC_10
	csrr t2, mbadaddr
#else
	csrr t2, mtval
#endif
	csrr t3, mcause
	SREG t0, 2 * REGSZ(x2)
	SREG s0, 32 * REGSZ(x2)
	SREG t1, 33 * REGSZ(x2)
	SREG t2, 34 * REGSZ(x2)
	SREG t3, 35 * REGSZ(x2)
	li x5, -1
	SREG x5, 36 * REGSZ(x2)
	move a0, sp
	jal riscv64_handle_exception
	csrr a0, mscratch
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
	LREG x10, 10 * REGSZ(a0)
	mret

	/*
	* The location of section
	*/
	.align 3
_image_start:
	RVPTR __image_start
_s_bss:
	RVPTR _sbss
_e_bss:
	RVPTR _ebss
_stack_start:
	RVPTR __stack_srv_start
_stack_end:
	RVPTR __stack_srv_end