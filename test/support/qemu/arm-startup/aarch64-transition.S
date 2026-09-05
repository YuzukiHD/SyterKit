/* SPDX-License-Identifier: GPL-2.0+ */

	.section .text, "ax", %progbits
	.globl _start
	.type _start, %function
_start:
	mrs x0, scr_el3
	mov x1, #(1 << 10)
	bic x0, x0, x1
	msr scr_el3, x0

	mov x0, #0x1d3
	msr spsr_el3, x0
	ldr x0, =AARCH32_ENTRY
	msr elr_el3, x0
	isb
	eret
	.size _start, . - _start
