/* SPDX-License-Identifier: GPL-2.0+ */

/* CPU_SOFT_ENTRY points here after a secondary CPU is powered on. */
	.section .text.smp_entry, "ax", %progbits
	.arm
	.align 2
	.global smp_secondary_entry
	.type smp_secondary_entry, %function

smp_secondary_entry:
	cpsid i
	mrc p15, 0, r0, c1, c0, 1
	orr r0, r0, #0x40
	mcr p15, 0, r0, c1, c0, 1
	dsb
	isb

	mrc p15, 0, r0, c1, c0, 0
	bic r0, r0, #(1 << 13)
	mcr p15, 0, r0, c1, c0, 0
	dsb
	isb

	mrc p15, 0, r4, c0, c0, 5
	and r4, r4, #3
	cmp r4, #0
	beq 1f
	ldr r0, =smp_secondary_stack_tops
	ldr sp, [r0, r4, lsl #2]
	mov r0, r4
	bl smp_secondary_main

1:
	wfi
	b 1b

	.ltorg
	.size smp_secondary_entry, .-smp_secondary_entry
