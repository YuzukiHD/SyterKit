/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file efex.S
 * @brief Returning RV32 eFEX entry using the Boot ROM stack.
 */

#define EFEX_FRAME_SIZE 176

	.section .text.entry, "ax", @progbits
	.balign 4
	.globl _start
	.type _start, @function
_start:
	addi sp, sp, -EFEX_FRAME_SIZE
	sw ra, 0(sp)
	sw gp, 8(sp)
	sw tp, 12(sp)
	sw t0, 16(sp)
	sw t1, 20(sp)
	sw t2, 24(sp)
	sw s0, 28(sp)
	sw s1, 32(sp)
	sw a0, 36(sp)
	sw a1, 40(sp)
	sw a2, 44(sp)
	sw a3, 48(sp)
	sw a4, 52(sp)
	sw a5, 56(sp)
	sw a6, 60(sp)
	sw a7, 64(sp)
	sw s2, 68(sp)
	sw s3, 72(sp)
	sw s4, 76(sp)
	sw s5, 80(sp)
	sw s6, 84(sp)
	sw s7, 88(sp)
	sw s8, 92(sp)
	sw s9, 96(sp)
	sw s10, 100(sp)
	sw s11, 104(sp)
	sw t3, 108(sp)
	sw t4, 112(sp)
	sw t5, 116(sp)
	sw t6, 120(sp)
	addi t0, sp, EFEX_FRAME_SIZE
	sw t0, 4(sp)

	csrr t0, mstatus
	sw t0, 124(sp)
	csrr t0, mie
	sw t0, 128(sp)
	csrr t0, mtvec
	sw t0, 132(sp)
	csrr t0, 0x307 /* mtvt */
	sw t0, 136(sp)
	csrr t0, satp
	sw t0, 140(sp)
	csrr t0, 0x7c0 /* mxstatus */
	sw t0, 144(sp)
	csrr t0, 0x7c1 /* mhcr */
	sw t0, 148(sp)
	csrr t0, 0x7c2 /* mcor */
	sw t0, 152(sp)
	csrr t0, 0x7c5 /* mhint */
	sw t0, 156(sp)

	csrci mstatus, 8
	csrw mie, zero
	jal .Lclear_bss
	jal main
	sw a0, 36(sp)
	jal syterkit_efex_arch_sync
	fence rw, rw
	fence.i
	.long 0x01b0000b /* sync.is */

	lw t0, 156(sp)
	csrw 0x7c5, t0
	lw t0, 152(sp)
	csrw 0x7c2, t0
	lw t0, 148(sp)
	csrw 0x7c1, t0
	lw t0, 144(sp)
	csrw 0x7c0, t0
	lw t0, 140(sp)
	csrw satp, t0
	sfence.vma
	lw t0, 136(sp)
	csrw 0x307, t0
	lw t0, 132(sp)
	csrw mtvec, t0
	lw t0, 128(sp)
	csrw mie, t0
	fence rw, rw
	fence.i
	.long 0x01b0000b /* sync.is */

	lw ra, 0(sp)
	lw gp, 8(sp)
	lw tp, 12(sp)
	lw t0, 16(sp)
	lw t1, 20(sp)
	lw t2, 24(sp)
	lw s0, 28(sp)
	lw s1, 32(sp)
	lw a0, 36(sp)
	lw a1, 40(sp)
	lw a2, 44(sp)
	lw a3, 48(sp)
	lw a4, 52(sp)
	lw a5, 56(sp)
	lw a6, 60(sp)
	lw a7, 64(sp)
	lw s2, 68(sp)
	lw s3, 72(sp)
	lw s4, 76(sp)
	lw s5, 80(sp)
	lw s6, 84(sp)
	lw s7, 88(sp)
	lw s8, 92(sp)
	lw s9, 96(sp)
	lw s10, 100(sp)
	lw s11, 104(sp)
	lw t3, 108(sp)
	lw t4, 112(sp)
	lw t5, 116(sp)
	lw t6, 120(sp)
	lw t0, 124(sp)
	csrw mstatus, t0
	lw t0, 16(sp)
	lw sp, 4(sp)
	jr ra

.Lclear_bss:
	la t0, _sbss
	la t1, _ebss
1:	bgeu t0, t1, 2f
	sw zero, 0(t0)
	addi t0, t0, 4
	j 1b
2:	ret

	.size _start, . - _start
