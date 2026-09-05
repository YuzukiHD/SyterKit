/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file efex.S
 * @brief Returning RV64 eFEX entry using the Boot ROM stack (C906).
 *
 * The eFEX image runs on the Boot ROM stack and returns control to the
 * caller, so every GPR and CSR touched here is saved and restored.  C906 C
 * code may use T-Head memory instructions, so the complete C9xx extension
 * state is enabled before any C code is called (mirrors start.S).
 *
 * C906 is always built RV64, so the frame uses 8-byte slots.
 */

#include <asm/trap.h>
#include <csr.h>

/* 31 GPR slots + 9 CSR slots, 8 bytes each; frame stays 16-byte aligned. */
#define EFEX_FRAME_SIZE 320

	.section .text.entry, "ax", @progbits
	.balign 4
	.globl _start
	.type _start, @function
_start:
	addi sp, sp, -EFEX_FRAME_SIZE
	sd ra, 0(sp)
	sd gp, 16(sp)
	sd tp, 24(sp)
	sd t0, 32(sp)
	sd t1, 40(sp)
	sd t2, 48(sp)
	sd s0, 56(sp)
	sd s1, 64(sp)
	sd a0, 72(sp)
	sd a1, 80(sp)
	sd a2, 88(sp)
	sd a3, 96(sp)
	sd a4, 104(sp)
	sd a5, 112(sp)
	sd a6, 120(sp)
	sd a7, 128(sp)
	sd s2, 136(sp)
	sd s3, 144(sp)
	sd s4, 152(sp)
	sd s5, 160(sp)
	sd s6, 168(sp)
	sd s7, 176(sp)
	sd s8, 184(sp)
	sd s9, 192(sp)
	sd s10, 200(sp)
	sd s11, 208(sp)
	sd t3, 216(sp)
	sd t4, 224(sp)
	sd t5, 232(sp)
	sd t6, 240(sp)
	addi t0, sp, EFEX_FRAME_SIZE
	sd t0, 8(sp)

	csrr t0, mstatus
	sd t0, 248(sp)
	csrr t0, mie
	sd t0, 256(sp)
	csrr t0, mtvec
	sd t0, 264(sp)
	csrr t0, 0x307 /* mtvt */
	sd t0, 272(sp)
	csrr t0, satp
	sd t0, 280(sp)
	csrr t0, 0x7c0 /* mxstatus */
	sd t0, 288(sp)
	csrr t0, 0x7c1 /* mhcr */
	sd t0, 296(sp)
	csrr t0, 0x7c2 /* mcor */
	sd t0, 304(sp)
	csrr t0, 0x7c5 /* mhint */
	sd t0, 312(sp)

	csrci mstatus, 8
	csrw mie, zero

	/* C906 C code may use the complete T-Head extension state. */
	li t0, MXSTATUS_C9XX
	csrs mxstatus, t0
	fence.i

	jal .Lclear_bss
	jal main
	sd a0, 72(sp)
	jal syterkit_efex_arch_sync
	fence rw, rw
	fence.i
	.long 0x01b0000b /* sync.is */

	ld t0, 312(sp)
	csrw 0x7c5, t0
	ld t0, 304(sp)
	csrw 0x7c2, t0
	ld t0, 296(sp)
	csrw 0x7c1, t0
	ld t0, 288(sp)
	csrw 0x7c0, t0
	ld t0, 280(sp)
	csrw satp, t0
	sfence.vma
	ld t0, 272(sp)
	csrw 0x307, t0
	ld t0, 264(sp)
	csrw mtvec, t0
	ld t0, 256(sp)
	csrw mie, t0
	fence rw, rw
	fence.i
	.long 0x01b0000b /* sync.is */

	ld ra, 0(sp)
	ld gp, 16(sp)
	ld tp, 24(sp)
	ld t0, 32(sp)
	ld t1, 40(sp)
	ld t2, 48(sp)
	ld s0, 56(sp)
	ld s1, 64(sp)
	ld a0, 72(sp)
	ld a1, 80(sp)
	ld a2, 88(sp)
	ld a3, 96(sp)
	ld a4, 104(sp)
	ld a5, 112(sp)
	ld a6, 120(sp)
	ld a7, 128(sp)
	ld s2, 136(sp)
	ld s3, 144(sp)
	ld s4, 152(sp)
	ld s5, 160(sp)
	ld s6, 168(sp)
	ld s7, 176(sp)
	ld s8, 184(sp)
	ld s9, 192(sp)
	ld s10, 200(sp)
	ld s11, 208(sp)
	ld t3, 216(sp)
	ld t4, 224(sp)
	ld t5, 232(sp)
	ld t6, 240(sp)

	/* Restore mstatus last, then reload t0 and sp before returning. */
	ld t0, 248(sp)
	csrw mstatus, t0
	ld t0, 32(sp)
	ld sp, 8(sp)
	jr ra

.Lclear_bss:
	la t0, _sbss
	la t1, _ebss
1:	bgeu t0, t1, 2f
	sd zero, 0(t0)
	addi t0, t0, 8
	bltu t0, t1, 1b
2:	ret

	.size _start, . - _start
