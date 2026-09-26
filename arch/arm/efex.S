/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file efex.S
 * @brief Returning ARM32 eFEX entry using the Boot ROM stack.
 */

	.arm
	.section .text.entry, "ax", %progbits
	.balign 4
	.globl _start
	.type _start, %function

_start:
	/* Keep the entry point at the image start and use the Boot ROM stack. */
	b	.Lreset

.Lreset:
	stmfd	sp!, {lr}
	bl	.Lclear_bss

	/* BootROM enters with the instruction cache disabled. */
	mrc	p15, 0, r0, c1, c0, 0
	orr	r0, r0, #(1 << 12)
	mcr	p15, 0, r0, c1, c0, 0
	isb

	bl	main
	ldmfd	sp!, {lr}
	bx	lr

.Lclear_bss:
	ldr	r0, =_sbss
	ldr	r1, =_ebss
	mov	r2, #0
	cmp	r0, r1
	strlo	r2, [r0], #4
	blo	.Lclear_bss
	bx	lr

	.size _start, . - _start
