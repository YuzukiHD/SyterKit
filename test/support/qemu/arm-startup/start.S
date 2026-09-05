/* SPDX-License-Identifier: GPL-2.0+ */

#include <linkage.h>

	.syntax unified
	.arm

	.section .boot0_head, "a", %progbits
	.space 64

	.section .text.bootstrap, "ax", %progbits
	.globl test_entry
	.type test_entry, %function
test_entry:
	ldr r0, =_sbss
	ldr r1, =_ebss
	ldr r2, =0xa5a5a5a5
1:
	cmp r0, r1
	bhs 2f
	str r2, [r0], #4
	b 1b
2:
	b _start
	.size test_entry, . - test_entry

	.section .text.test_capture, "ax", %progbits
	.globl set_timer_count
	.type set_timer_count, %function
set_timer_count:
	mov r4, lr
	mrs r5, cpsr

	ldr r0, =startup_timer_seen
	ldr r1, =0x54494d45
	str r1, [r0]

	ldr r0, =startup_cpsr
	str r5, [r0]
	ldr r0, =startup_svc_sp
	str sp, [r0]

	mrc p15, 0, r1, c12, c0, 0
	ldr r0, =startup_vbar
	str r1, [r0]
	mrc p15, 0, r1, c1, c0, 0
	ldr r0, =startup_sctlr
	str r1, [r0]
	mrc p15, 0, r1, c1, c0, 2
	ldr r0, =startup_cpacr
	str r1, [r0]
	vmrs r1, fpexc
	ldr r0, =startup_fpexc
	str r1, [r0]

	mov r2, #0x5a
	vdup.32 q0, r2
	vmov r1, s0
	ldr r0, =startup_neon_value
	str r1, [r0]

	bic r6, r5, #ARMV7_MODE_MASK
	orr r6, r6, #ARMV7_UND_MODE
	msr cpsr_c, r6
	ldr r0, =startup_und_sp
	str sp, [r0]

	bic r6, r5, #ARMV7_MODE_MASK
	orr r6, r6, #ARMV7_ABT_MODE
	msr cpsr_c, r6
	ldr r0, =startup_abt_sp
	str sp, [r0]

	bic r6, r5, #ARMV7_MODE_MASK
	orr r6, r6, #ARMV7_IRQ_MODE
	msr cpsr_c, r6
	ldr r0, =startup_irq_sp
	str sp, [r0]

	bic r6, r5, #ARMV7_MODE_MASK
	orr r6, r6, #ARMV7_FIQ_MODE
	msr cpsr_c, r6
	ldr r0, =startup_fiq_sp
	str sp, [r0]

	msr cpsr_c, r5
	mov lr, r4
	bx lr
	.size set_timer_count, . - set_timer_count

	.globl semihost_write0
	.type semihost_write0, %function
semihost_write0:
	mov r1, r0
	mov r0, #4
	svc #0x123456
	bx lr
	.size semihost_write0, . - semihost_write0
