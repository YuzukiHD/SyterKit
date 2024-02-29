/* SPDX-License-Identifier: Apache-2.0 */

#include <linkage.h>

.macro save_regs
	str lr, [sp, #-4]
	mrs lr, spsr_all
	str lr, [sp, #-8]
	str r1, [sp, #-12]
	str r0, [sp, #-16]
	mov r0, sp
	cps #0x13
	ldr r1, [r0, #-4]
	str r1, [sp, #-4]!
	ldr r1, [r0, #-8]
	str r1, [sp, #-(4 * 16)]
	ldr r1, [r0, #-12]
	ldr r0, [r0, #-16]
	stmdb sp, {r0 - r14}^
	sub sp, sp, #(4 * 16)
	ldr r4, [sp]
	and r0, r4, #0x1f
	cmp r0, #0x10
	beq 10f
	cmp r0, #0x13
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
	and r0, r1, #0x1f
	cmp r0, #0x10
	beq 20f
	cmp r0, #0x13
	beq 21f
	b .
20:	ldr lr, [r12, #(4 * 15)]
	ldmia r12, {r0 - r14}^
	movs pc, lr
21:	ldm r12, {r0 - r15}^
	mov r0, r0
.endm

.arm
.globl _start
.text

_start:
	b reset
	
	.align 5
_vector:
	b reset
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

reset:
	/* Enter svc mode cleanly and mask interrupts */
	mrs r0, cpsr
	bic r0, r0, #ARMV7_MODE_MASK
	orr r0, r0, #ARMV7_SVC_MODE
	orr r0, r0, #(ARMV7_IRQ_MASK | ARMV7_FIQ_MASK)
	bic r0, r0, #(1<<9)     @set little-endian
	msr cpsr_c, r0

	/* Set vector base address register */
	ldr r0, =_vector
	mcr p15, 0, r0, c12, c0, 0
	mrc p15, 0, r0, c1, c0, 0
	bic r0, #(1 << 13)
	mcr p15, 0, r0, c1, c0, 0

	mrc p15, 0, r0, c1, c0, 0
	bic r0, r0, #0x00002000     @ clear bits 13 (--V-)
	bic r0, r0, #0x00000007     @ clear bits 2:0 (-CAM)
	orr r0, r0, #0x00000800     @ set bit 11 (Z---) BTB
	bic r0, r0, #0x00001000     @ clear bit 12 (I) I-cache
	mcr p15, 0, r0, c1, c0, 0

	/* Set stack pointer */
	/* Initialize UND stacks */
	mrs r0, cpsr
	bic r0, r0, #ARMV7_MODE_MASK
	orr r1, r0, #ARMV7_UND_MODE
	msr cpsr_cxsf, r1
	ldr sp, _stack_und_end

	/* Initialize ABT stacks */
	mrs r0, cpsr
	bic r0, r0, #ARMV7_MODE_MASK
	orr r1, r0, #ARMV7_ABT_MODE
	msr cpsr_cxsf, r1
	ldr sp, _stack_abt_end

	/* Initialize IRQ stacks */
	mrs r0, cpsr
	bic r0, r0, #ARMV7_MODE_MASK
	orr r1, r0, #ARMV7_IRQ_MODE
	msr cpsr_cxsf, r1
	ldr sp, _stack_irq_end

	/* Initialize FIQ stacks */
	mrs r0, cpsr
	bic r0, r0, #ARMV7_MODE_MASK
	orr r1, r0, #ARMV7_FIQ_MODE
	msr cpsr_cxsf, r1
	ldr sp, _stack_fiq_end

	/* Initialize SVC stacks */
	mrs r0, cpsr
	bic r0, r0, #ARMV7_MODE_MASK
	orr r1, r0, #ARMV7_SVC_MODE
	msr cpsr_cxsf, r1
	ldr sp, _stack_srv_end

	bl  clear_bss

	/*
	* disable interrupts (FIQ and IRQ), also set the cpu to SVC32 mode,
	* except if in HYP mode already
	*/
	mrs     r0, cpsr
	and     r1, r0, #ARMV7_MODE_MASK           @ mask mode bits
	teq     r1, #0x1a               @ test for HYP mode
	bicne   r0, r0, #ARMV7_MODE_MASK           @ clear all mode bits
	orrne   r0, r0, #ARMV7_SVC_MODE           @ set SVC mode
	orr     r0, r0, #0xc0           @ disable FIQ and IRQ
	msr     cpsr,r0

	/* set cntfrq to 24M */
	ldr r0, =24000000
	mcr p15, 0, r0, c14, c0, 0

	bl set_timer_count

	bl main

clear_bss:
	ldr     r0, =_sbss
	ldr     r1, =_ebss
	mov     r2, #0

clbss_1:
	stmia   r0!, {r2}
	cmp r0, r1
	blt clbss_1

	mov pc, lr

	/*
	* Exception handlers
	*/
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

	/*
	* The location of section
	*/
 	.align 4
_stack_und_end:
	.long __stack_und_end
_stack_abt_end:
	.long __stack_abt_end
_stack_irq_end:
	.long __stack_irq_end
_stack_fiq_end:
	.long __stack_fiq_end
_stack_srv_end:
	.long __stack_srv_end