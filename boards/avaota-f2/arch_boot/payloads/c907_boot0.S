/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Start the C907 core from the E907 boot path.
 *
 * This is derived from the optimized (-Os) output of the vendor
 * c907_boot0_main.c.
 * ABI register names retain the compiler's compact RVC allocation:
 *
 *   a6  C907 run address (payload + 0x100)
 *   a0-a5  MMIO addresses, values, and masks
 */
	.section .text, "ax", @progbits
	.balign 0x2
	.option nopic
	.globl c907_boot0
	.type c907_boot0, @function
c907_boot0:
	/* The SyterKit entry follows the fixed-size payload. */
	auipc	a6, 0x0
	addi	a6, a6, 0x100

	/* Disable E907 machine interrupts. */
	li	a2, 0x8
	csrc	mstatus, a2

	/* Initialize the C907 clock/reset controls. */
	li	a5, 0xf8216000
	li	a4, 0x02001000
	addi	a5, a5, 0x310
	sw	a5, 0x20(a4)

	/* Select the 200 MHz peripheral clock and enable the PIC clock. */
	addi	a4, a4, 0x508
	lw	a5, 0x0(a4)
	li	a3, 0x7f000000
	addi	a3, a3, -0x1
	and	a5, a5, a3
	li	a3, 0x80000000
	or	a5, a5, a3
	sw	a5, 0x0(a4)

	/* Enable the CPU configuration bus and release its reset. */
	li	a4, 0x02001000
	addi	a4, a4, 0x50c
	lw	a1, 0x0(a4)
	li	a5, 0x02001000
	li	a0, 0x00010000
	addi	a5, a5, 0x504
	addi	a0, a0, 0x1
	lw	a3, 0x0(a5)
	or	a1, a1, a0
	sw	a1, 0x0(a4)
	li	a4, 0x16aa1000
	addi	a4, a4, 0xc
	or	a4, a3, a4
	sw	a4, 0x0(a5)
	fence	ow, ow

	/* Select the C907 reset mode. */
	li	a3, 0x08008000
	addi	a3, a3, 0x108
	lw	a4, 0x0(a3)
	andi	a4, a4, -0x301
#if C907_CORE_MODE == 0x2
	ori	a4, a4, 0x200
#else
	ori	a4, a4, 0x100
#endif
	sw	a4, 0x0(a3)
	fence	ow, ow

	/* Program the address fetched by C907 after reset. */
	li	a4, 0x08008000
	sw	a6, 0x100(a4)
	fence	ow, ow

	/* Configure the C907 CPU clock and release its clock/reset gates. */
	li	a4, 0x02001000
	addi	a4, a4, 0x500
	lw	a3, 0x0(a4)
	li	a1, 0xfffd0000
	addi	a1, a1, -0x1
	and	a3, a3, a1
	sw	a3, 0x0(a4)

	lw	a4, 0x0(a5)
	li	a3, 0x16aa0000
	addi	a3, a3, 0x103
	or	a4, a4, a3
	sw	a4, 0x0(a5)
	fence	ow, ow

	/* Release C907 CPU0, then CPU1. */
	lw	a4, 0x0(a5)
	li	a3, 0x16aa0000
	addi	a3, a3, 0x10
	or	a4, a4, a3
	sw	a4, 0x0(a5)
	fence	ow, ow

	lw	a4, 0x0(a5)
	li	a3, 0x16aa0000
	addi	a3, a3, 0x20
	or	a4, a4, a3
	sw	a4, 0x0(a5)

	/* Enter E907 light sleep after handing control to C907. */
	li	a2, 0xc
	csrrc	a2, mexstatus, a2
	li	a2, 0x4
	csrrs	a2, mexstatus, a2
1:	wfi
	j	1b
	.size c907_boot0, . - c907_boot0
	.section .note.GNU-stack,"",@progbits
