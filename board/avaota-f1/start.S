/* SPDX-License-Identifier: GPL-2.0+ */

#include <linkage.h>
#include <csr.h>

	.text
	.align 4
	.globl _start
_start:
	/* This chip enable mie in BROM, We needs to close it right now */
	csrc mstatus, MSTATUS_MIE

	/* disable interrupt */
	csrw mie, zero
	li 	x1, 0
	li 	x2, 0
	li 	x3, 0
	li 	x4, 0
	li 	x5, 0
	li 	x6, 0
	li 	x7, 0
	li 	x8, 0
	li 	x9, 0
	li 	x10, 0
	li 	x11, 0
	li 	x12, 0
	li 	x13, 0
	li 	x14, 0
	li 	x15, 0
	li 	x16, 0
	li 	x17, 0
	li 	x18, 0
	li 	x19, 0
	li 	x20, 0
	li 	x21, 0
	li 	x22, 0
	li 	x23, 0
	li 	x24, 0
	li 	x25, 0
	li 	x26, 0
	li 	x27, 0
	li 	x28, 0
	li 	x29, 0
	li 	x30, 0
	li 	x31, 0

	/* enable FPU and accelerator if present */
	li 	t0, MSTATUS_FS | MSTATUS_XS
	csrs mstatus, t0

	/* enable MXSTATUS.MM MXSTATUS.THEADISAEE bit[22] */
	li 	t0, MXSTATUS_MM | MXSTATUS_THEADISAEE
	csrs mxstatus, t0

	/* invaild ICACHE/DCACHE/BTB/BHT */
	fence.i

	/* Setup exception vectors */
	la a0, irq_handler
	ori	a0, a0, 3
	csrw mtvec, a0

	/* Setup int vectors */
	la a0, vectors
	csrw mtvt, a0

	/* Setup stack */
	la sp, __stack_srv_end
	csrw mscratch, sp

	/* clear bss */
	jal clear_bss

	jal set_timer_count

	jal cache_init

	jal icache_enable

	jal dcache_enable

	fence.i

	jal main

clear_bss:
	la	t0, _sbss
	la	t1, _ebss

clbss_1:
	sw zero, 0(t0)
	addi t0, t0, 4
	blt t0, t1, clbss_1
	ret

	/*
	* IRQ Handler
	*/
	.align 2
	.globl vectors
irq_handler:
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
	jal riscv_handle_exception
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
	* Exception vectors.
	*/
.section .vectors, "aw", @progbits
	.align 6
	.globl vectors
	.type vectors, @object
vectors:
	.rept (16)
    .long irq_handler
	.endr
