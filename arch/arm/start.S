/* SPDX-License-Identifier: GPL-2.0+ */

#include <linkage.h>

.macro save_regs
	str lr, [sp, #-4]
	mrs lr, spsr_all
	str lr, [sp, #-8]
	str r1, [sp, #-12]
	str r0, [sp, #-16]
	mov r0, sp
	cps #ARMV7_SVC_MODE
	ldr r1, [r0, #-4]
	str r1, [sp, #-4]!
	ldr r1, [r0, #-8]
	str r1, [sp, #-(4 * 16)]
	ldr r1, [r0, #-12]
	ldr r0, [r0, #-16]
	stmdb sp, {r0 - r14}^
	sub sp, sp, #(4 * 16)
	ldr r4, [sp]
	and r0, r4, #ARMV7_MODE_MASK
	cmp r0, #ARMV7_USR_MODE
	beq 10f
	cmp r0, #ARMV7_SVC_MODE
	beq 11f
	b .
11:	add r1, sp, #(4 * 17)
	str r1, [sp, #(4 * 14)]
	str lr, [sp, #(4 * 15)]
10:	add r1, sp, #(4 * 17)
	str r1, [sp, #-4]!
	mov r0, sp
.endm

.macro restore_regs
	mov r12, sp
	ldr sp, [r12], #4
	ldr r1, [r12], #4
	msr spsr_cxsf, r1
	and r0, r1, #ARMV7_MODE_MASK
	cmp r0, #ARMV7_USR_MODE
	beq 20f
	cmp r0, #ARMV7_SVC_MODE
	beq 21f
	b .
20:	ldr lr, [r12, #(4 * 15)]
	ldmia r12, {r0 - r14}^
	movs pc, lr
21:	ldm r12, {r0 - r15}^
	mov r0, r0
.endm

	.arm
	.section .text.entry, "ax", %progbits
	.globl _start
	.type _start, %function
_start:
	b reset

	.align 5
_vector:
	b reset
#ifndef CONFIG_ARCH_EXCEPTION
	ldr pc, _unhandled_exception
	ldr pc, _unhandled_exception
	ldr pc, _unhandled_exception
	ldr pc, _unhandled_exception
	ldr pc, _unhandled_exception
	ldr pc, _unhandled_exception
	ldr pc, _unhandled_exception

_unhandled_exception:
	.word unhandled_exception
#else
	ldr pc, _undefined_instruction
	ldr pc, _software_interrupt
	ldr pc, _prefetch_abort
	ldr pc, _data_abort
	ldr pc, _not_used
	ldr pc, _irq
	ldr pc, _fiq

_undefined_instruction:
	.word undefined_instruction
_software_interrupt:
	.word software_interrupt
_prefetch_abort:
	.word prefetch_abort
_data_abort:
	.word data_abort
_not_used:
	.word not_used
_irq:
	.word irq
_fiq:
	.word fiq
#endif

reset:
	/* Enter SVC mode and mask IRQ/FIQ before using banked stacks. */
	mrs r0, cpsr
	bic r0, r0, #ARMV7_MODE_MASK
	orr r0, r0, #ARMV7_SVC_MODE
	orr r0, r0, #(ARMV7_IRQ_MASK | ARMV7_FIQ_MASK)
	bic r0, r0, #(1 << 9)
	msr cpsr_c, r0

	ldr r0, =_vector
	mcr p15, 0, r0, c12, c0, 0

	/* Disable MMU and caches while preserving the remaining SCTLR state. */
	mrc p15, 0, r0, c1, c0, 0
	bic r0, r0, #0x00002000
	bic r0, r0, #0x00000007
	orr r0, r0, #0x00000800
	bic r0, r0, #0x00001000
	mcr p15, 0, r0, c1, c0, 0
	isb

	bl neon_enable

#ifdef CONFIG_ARCH_EXCEPTION
	mrs r0, cpsr
	bic r0, r0, #ARMV7_MODE_MASK
	orr r1, r0, #ARMV7_UND_MODE
	msr cpsr_cxsf, r1
	ldr sp, =__stack_und_end

	mrs r0, cpsr
	bic r0, r0, #ARMV7_MODE_MASK
	orr r1, r0, #ARMV7_ABT_MODE
	msr cpsr_cxsf, r1
	ldr sp, =__stack_abt_end

	mrs r0, cpsr
	bic r0, r0, #ARMV7_MODE_MASK
	orr r1, r0, #ARMV7_IRQ_MODE
	msr cpsr_cxsf, r1
	ldr sp, =__stack_irq_end

	mrs r0, cpsr
	bic r0, r0, #ARMV7_MODE_MASK
	orr r1, r0, #ARMV7_FIQ_MODE
	msr cpsr_cxsf, r1
	ldr sp, =__stack_fiq_end
#endif

	mrs r0, cpsr
	bic r0, r0, #ARMV7_MODE_MASK
	orr r1, r0, #ARMV7_SVC_MODE
	msr cpsr_cxsf, r1
	ldr sp, =__stack_srv_end

	bl clear_bss

#ifdef CONFIG_CPU_CORTEX_A7
	ldr r0, =24000000
	mcr p15, 0, r0, c14, c0, 0
#endif
	bl set_timer_count

	bl main

1:	wfi
	b 1b

	.globl neon_enable
	.type neon_enable, %function
neon_enable:
	mrc p15, 0, r0, c1, c0, 2
	orr r0, r0, #(0xf << 20)
	mcr p15, 0, r0, c1, c0, 2
	isb
	mov r0, #0x40000000
	vmsr fpexc, r0
	bx lr

clear_bss:
	ldr r0, =_sbss
	ldr r1, =_ebss
	mov r2, #0
	cmp r0, r1
	bhs 2f
1:	stmia r0!, {r2}
	cmp r0, r1
	blo 1b
2:	bx lr

#ifndef CONFIG_ARCH_EXCEPTION
	.align 5
unhandled_exception:
	wfi
	b unhandled_exception
#else
	.align 5
undefined_instruction:
	sub lr, lr, #4
	save_regs
	bl arm32_do_undefined_instruction
	restore_regs

	.align 5
software_interrupt:
	sub lr, lr, #4
	save_regs
	bl arm32_do_software_interrupt
	restore_regs

	.align 5
prefetch_abort:
	sub lr, lr, #4
	save_regs
	bl arm32_do_prefetch_abort
	restore_regs

	.align 5
data_abort:
	sub lr, lr, #8
	save_regs
	bl arm32_do_data_abort
	restore_regs

	.align 5
not_used:
	b .

	.align 5
irq:
	sub lr, lr, #4
	save_regs
	bl arm32_do_irq
	restore_regs

	.align 5
fiq:
	sub lr, lr, #4
	save_regs
	bl arm32_do_fiq
	restore_regs
#endif
