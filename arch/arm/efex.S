/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file
 * @brief Returning ARM32 eFEX entry using the Boot ROM stack.
 */

	.arm
	.section .text.entry, "ax", %progbits
	.globl _start
	.type _start, %function

/* Twelve control words follow the fourteen saved general registers. */
#define EFEX_CONTROL_BYTES 48

_start:
	/* Keep the incoming stack and all caller state in a 104-byte frame. */
	stmdb sp!, {r0-r12, lr}
	mrs r0, cpsr
	stmdb sp!, {r0}
	mrc p15, 0, r0, c1, c0, 0
	stmdb sp!, {r0}
	mrc p15, 0, r0, c1, c0, 1
	stmdb sp!, {r0}
	mrc p15, 0, r0, c12, c0, 0
	stmdb sp!, {r0}
	mrrc p15, 0, r0, r1, c2
	stmdb sp!, {r0}
	stmdb sp!, {r1}
	mrrc p15, 1, r0, r1, c2
	stmdb sp!, {r0}
	stmdb sp!, {r1}
	mrc p15, 0, r0, c2, c0, 2
	stmdb sp!, {r0}
	mrc p15, 0, r0, c3, c0, 0
	stmdb sp!, {r0}
	mrc p15, 0, r1, c1, c0, 2
	stmdb sp!, {r1}
	orr r0, r1, #(0xf << 20)
	mcr p15, 0, r0, c1, c0, 2
	isb
	vmrs r0, fpexc
	stmdb sp!, {r0}
	orr r0, r0, #0x40000000
	vmsr fpexc, r0

	cpsid if
	bl .Lclear_bss
	bl main

	/* The saved r0 slot becomes the ABI return value. */
	str r0, [sp, #(EFEX_CONTROL_BYTES + 0)]
	bl syterkit_efex_arch_sync
	dsb sy
	isb

	ldmia sp!, {r0}
	vmsr fpexc, r0
	ldmia sp!, {r0}
	mcr p15, 0, r0, c1, c0, 2
	ldmia sp!, {r0}
	mcr p15, 0, r0, c3, c0, 0
	ldmia sp!, {r0}
	mcr p15, 0, r0, c2, c0, 2
	ldmia sp!, {r1}
	ldmia sp!, {r0}
	mcrr p15, 1, r0, r1, c2
	ldmia sp!, {r1}
	ldmia sp!, {r0}
	mcrr p15, 0, r0, r1, c2
	ldmia sp!, {r0}
	mcr p15, 0, r0, c12, c0, 0
	ldmia sp!, {r0}
	mcr p15, 0, r0, c1, c0, 1
	ldmia sp!, {r0}
	mcr p15, 0, r0, c1, c0, 0
	dsb sy
	isb
	ldmia sp!, {r1}
	msr cpsr_cxsf, r1
	ldmia sp!, {r0-r12, lr}
	bx lr

.Lclear_bss:
	ldr r0, =_sbss
	ldr r1, =_ebss
	mov r2, #0
1:	cmp r0, r1
	strlo r2, [r0], #4
	blo 1b
	bx lr

	.size _start, . - _start
