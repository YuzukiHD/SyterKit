/* SPDX-License-Identifier: GPL-2.0+ */

#include <asm/trap.h>
#include <csr.h>

	.section .text.entry, "ax", @progbits
	.balign 4
	.globl _start
	.type _start, @function
_start:
	csrc mstatus, MSTATUS_MIE
	csrw mie, zero

	la sp, __stack_srv_end
	la a0, vectors
	csrw mtvec, a0
	jal clear_bss

	/* Enable T-Head ISA extensions and misaligned access handling. */
	li t1, (1 << 22) | (1 << 21) | (1 << 15)
	csrs mxstatus, t1

	/* Invalidate I-cache, D-cache, BTB and BHT. */
	li t2, 0x30013
	csrs mcor, t2

	/* Give machine mode access to the 32-bit physical address space. */
	li t0, (0xfffffffc >> 2)
	csrw pmpaddr0, t0
	li t0, (1 << 3) | (1 << 2) | (1 << 1) | (1 << 0)
	csrw pmpcfg0, t0

	csrw mideleg, zero
	csrw medeleg, zero
	csrw mip, zero

	li t0, MSTATUS_FS | MSTATUS_XS | (3 << 23)
	csrs mstatus, t0

	jal set_timer_count
	jal do_initcalls
	jal main

1:	wfi
	j 1b

clear_bss:
	la t0, _sbss
	la t1, _ebss
	bgeu t0, t1, 2f
1:	sw zero, 0(t0)
	addi t0, t0, 4
	bltu t0, t1, 1b
2:	ret

	.balign 4
	.globl vectors
	.type vectors, @function
vectors:
	SAVE_ALL
	jal riscv64_handle_exception
	RESTORE_ALL
