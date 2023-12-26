/* SPDX-License-Identifier: Apache-2.0 */
#include <linkage.h>

#define  ARMV7_USR_MODE           0x10
#define  ARMV7_FIQ_MODE           0x11
#define  ARMV7_IRQ_MODE           0x12
#define  ARMV7_SVC_MODE           0x13
#define  ARMV7_MON_MODE        	  0x16
#define  ARMV7_ABT_MODE           0x17
#define  ARMV7_UND_MODE           0x1b
#define  ARMV7_SYSTEM_MODE        0x1f
#define  ARMV7_MODE_MASK          0x1f
#define  ARMV7_FIQ_MASK           0x40
#define  ARMV7_IRQ_MASK           0x80

.arm
.globl reset
.text

reset:
	/* Boot head information for BROM */
	.long 0xea000016
	.byte 'e', 'G', 'O', 'N', '.', 'B', 'T', '0'
	.long 0x12345678                                /* checksum */
	.long __spl_size                                /* spl size */
	.long 0x30                                      /* boot header size */
	.long 0x30303033                                /* boot header version */
	.long 0x00020000                                /* return value */
	.long 0x00028000                                /* run address */
	.long 0x0                                       /* eGON version */
	.byte 0x00, 0x00, 0x00, 0x00    				/* platform information - 8byte */
	.byte 0x34, 0x2e, 0x30, 0x00

	mrs r0, cpsr
	bic r0, r0, #ARMV7_MODE_MASK
	orr r0, r0, #ARMV7_SVC_MODE
	orr r0, r0, #(ARMV7_IRQ_MASK | ARMV7_FIQ_MASK)
	bic r0, r0, #(1<<9)     @set little-endian
	msr cpsr_c, r0

	mrc	p15, 0, r0, c1, c0, 0
	bic	r0, r0, #0x00002000	@ clear bits 13 (--V-)
	bic	r0, r0, #0x00000007	@ clear bits 2:0 (-CAM)
	orr	r0, r0, #0x00000800	@ set bit 11 (Z---) BTB
	bic	r0, r0, #0x00001000	@ clear bit 12 (I) I-cache
	mcr	p15, 0, r0, c1, c0, 0

	/* Set stack pointer */
	ldr sp, =__stack_srv_end
	
	bl  clear_bss
	bl  main

clear_bss:
	ldr     r0, =_sbss
	ldr     r1, =_ebss
	mov     r2, #0

clbss_1:
	stmia   r0!, {r2}
	cmp r0, r1
	blt clbss_1

	mov pc, lr

