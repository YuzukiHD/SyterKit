/* SPDX-License-Identifier: Apache-2.0 */

#include <linkage.h>

// Macro to save register values onto the stack
.macro save_regs
    str lr, [sp, #-4]      // Save value of lr register onto the stack
    mrs lr, spsr_all       // Save value of spsr_all register onto lr register
    str lr, [sp, #-8]      // Save value of lr register onto the stack
    str r1, [sp, #-12]     // Save value of r1 register onto the stack
    str r0, [sp, #-16]     // Save value of r0 register onto the stack
    mov r0, sp             // Copy value of sp register to r0 register
    cps #0x13              // Switch to SVC mode
    ldr r1, [r0, #-4]      // Load value of r1 register from stack to r1 register
    str r1, [sp, #-4]!     // Save value of r1 register onto the stack and update stack pointer
    ldr r1, [r0, #-8]      // Load value of r1 register from stack to r1 register
    str r1, [sp, #-(4 * 16)] // Save value of r1 register onto the stack
    ldr r1, [r0, #-12]     // Load value of r1 register from stack to r1 register
    ldr r0, [r0, #-16]     // Load value of r0 register from stack to r0 register
    stmdb sp, {r0 - r14}^  // Save values of registers r0-r14 onto the stack
    sub sp, sp, #(4 * 16)  // Update stack pointer
    ldr r4, [sp]           // Load value of r4 register from stack to r4 register
    and r0, r4, #0x1f      // Copy the lower 5 bits of r4 register to r0 register
    cmp r0, #0x10          // Compare value of r0 register with 0x10
    beq 10f                // If equal, jump to label 10
    cmp r0, #0x13          // Compare value of r0 register with 0x13
    beq 11f                // If equal, jump to label 11
    b .                    // Jump to current location
11: add r1, sp, #(4 * 17) // Add (4 * 17) to sp register and copy result to r1 register
    str r1, [sp, #(4 * 14)] // Save value of r1 register onto the stack
    str lr, [sp, #(4 * 15)] // Save value of lr register onto the stack
10: add r1, sp, #(4 * 17) // Add (4 * 17) to sp register and copy result to r1 register
    str r1, [sp, #-4]!     // Save value of r1 register onto the stack and update stack pointer
    mov r0, sp             // Copy value of sp register to r0 register
.endm

// Macro to restore register values from the stack
.macro restore_regs
    mov r12, sp            // Copy value of sp register to r12 register
    ldr sp, [r12], #4      // Load value from memory pointed by r12 register to sp register and update r12 register
    ldr r1, [r12], #4      // Load value from memory pointed by r12 register to r1 register and update r12 register
    msr spsr_cxsf, r1      // Write value of r1 register to spsr_cxsf register
    and r0, r1, #0x1f      // Copy the lower 5 bits of r1 register to r0 register
    cmp r0, #0x10          // Compare value of r0 register with 0x10
    beq 20f                // If equal, jump to label 20
    cmp r0, #0x13          // Compare value of r0 register with 0x13
    beq 21f                // If equal, jump to label 21
    b .                    // Jump to current location
20: ldr lr, [r12, #(4 * 15)] // Load value from memory pointed by r12 register to lr register
    ldmia r12, {r0 - r14}^ // Load values of registers r0-r14 from memory pointed by r12 register and update r12 register
    movs pc, lr            // Set value of PC register to value of lr register
21: ldm r12, {r0 - r15}^  // Load values of registers r0-r15 from memory pointed by r12 register
    mov r0, r0             // Copy value of r0 register to r0 register
.endm

#define  ARMV7_USR_MODE        0x10
#define  ARMV7_FIQ_MODE        0x11
#define  ARMV7_IRQ_MODE        0x12
#define  ARMV7_SVC_MODE        0x13
#define  ARMV7_MON_MODE        0x16
#define  ARMV7_ABT_MODE        0x17
#define  ARMV7_UND_MODE        0x1b
#define  ARMV7_SYSTEM_MODE     0x1f
#define  ARMV7_MODE_MASK       0x1f
#define  ARMV7_FIQ_MASK        0x40
#define  ARMV7_IRQ_MASK        0x80

.arm
.globl _start
.text

_start:
	/* Boot head information for BROM */
	.long 0xea000016
	.byte 'e', 'G', 'O', 'N', '.', 'B', 'T', '0'
	.long 0x12345678                                /* checksum */
	.long __spl_size                                /* spl size */
	.long 0x30                                      /* boot header size */
	.long 0x30303033                                /* boot header version */
	.long 0x00044000                                /* return value */
	.long 0x00044000                                /* run address */
	.long 0x0                                       /* eGON version */
	.byte 0x00, 0x00, 0x00, 0x00    				/* platform information - 8byte */
	.byte 0x34, 0x2e, 0x30, 0x00

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
	ldr sp, =__stack_srv_end

	bl  clear_bss

	/*
	* disable interrupts (FIQ and IRQ), also set the cpu to SVC32 mode,
	* except if in HYP mode already
	*/
	mrs     r0, cpsr
	and     r1, r0, #0x1f           @ mask mode bits
	teq     r1, #0x1a               @ test for HYP mode
	bicne   r0, r0, #0x1f           @ clear all mode bits
	orrne   r0, r0, #0x13           @ set SVC mode
	orr     r0, r0, #0xc0           @ disable FIQ and IRQ
	msr     cpsr,r0

	@ set cntfrq to 24M

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
