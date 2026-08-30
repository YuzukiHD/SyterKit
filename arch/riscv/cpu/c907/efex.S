/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file efex.S
 * @brief Returning RISC-V eFEX entry using the Boot ROM stack (C907).
 *
 * The eFEX image runs on the Boot ROM stack and returns control to the
 * caller, so every GPR and CSR touched here is saved and restored.  C907 C
 * code may use T-Head memory instructions, so the complete C9xx extension
 * state is enabled before any C code is called (mirrors start.S).
 *
 * The frame layout adapts to the active ISA so the same entry serves both
 * RV32 and RV64 builds.
 */

#include <asm/trap.h>
#include <csr.h>

#ifdef CONFIG_ARCH_RISCV64
#define EFEX_REGSZ 8
#define REG_S sd
#define REG_L ld
#else
#define EFEX_REGSZ 4
#define REG_S sw
#define REG_L lw
#endif

/* 31 GPR slots + 9 CSR slots; the frame stays 16-byte aligned for both ISAs. */
#define EFEX_FRAME_SIZE (40 * EFEX_REGSZ)

	.section .text.entry, "ax", @progbits
	.balign 4
	.globl _start
	.type _start, @function
_start:
	addi sp, sp, -EFEX_FRAME_SIZE
	REG_S ra, (0 * EFEX_REGSZ)(sp)
	REG_S gp, (2 * EFEX_REGSZ)(sp)
	REG_S tp, (3 * EFEX_REGSZ)(sp)
	REG_S t0, (4 * EFEX_REGSZ)(sp)
	REG_S t1, (5 * EFEX_REGSZ)(sp)
	REG_S t2, (6 * EFEX_REGSZ)(sp)
	REG_S s0, (7 * EFEX_REGSZ)(sp)
	REG_S s1, (8 * EFEX_REGSZ)(sp)
	REG_S a0, (9 * EFEX_REGSZ)(sp)
	REG_S a1, (10 * EFEX_REGSZ)(sp)
	REG_S a2, (11 * EFEX_REGSZ)(sp)
	REG_S a3, (12 * EFEX_REGSZ)(sp)
	REG_S a4, (13 * EFEX_REGSZ)(sp)
	REG_S a5, (14 * EFEX_REGSZ)(sp)
	REG_S a6, (15 * EFEX_REGSZ)(sp)
	REG_S a7, (16 * EFEX_REGSZ)(sp)
	REG_S s2, (17 * EFEX_REGSZ)(sp)
	REG_S s3, (18 * EFEX_REGSZ)(sp)
	REG_S s4, (19 * EFEX_REGSZ)(sp)
	REG_S s5, (20 * EFEX_REGSZ)(sp)
	REG_S s6, (21 * EFEX_REGSZ)(sp)
	REG_S s7, (22 * EFEX_REGSZ)(sp)
	REG_S s8, (23 * EFEX_REGSZ)(sp)
	REG_S s9, (24 * EFEX_REGSZ)(sp)
	REG_S s10, (25 * EFEX_REGSZ)(sp)
	REG_S s11, (26 * EFEX_REGSZ)(sp)
	REG_S t3, (27 * EFEX_REGSZ)(sp)
	REG_S t4, (28 * EFEX_REGSZ)(sp)
	REG_S t5, (29 * EFEX_REGSZ)(sp)
	REG_S t6, (30 * EFEX_REGSZ)(sp)
	addi t0, sp, EFEX_FRAME_SIZE
	REG_S t0, (1 * EFEX_REGSZ)(sp)

	csrr t0, mstatus
	REG_S t0, (31 * EFEX_REGSZ)(sp)
	csrr t0, mie
	REG_S t0, (32 * EFEX_REGSZ)(sp)
	csrr t0, mtvec
	REG_S t0, (33 * EFEX_REGSZ)(sp)
	csrr t0, 0x307 /* mtvt */
	REG_S t0, (34 * EFEX_REGSZ)(sp)
	csrr t0, satp
	REG_S t0, (35 * EFEX_REGSZ)(sp)
	csrr t0, 0x7c0 /* mxstatus */
	REG_S t0, (36 * EFEX_REGSZ)(sp)
	csrr t0, 0x7c1 /* mhcr */
	REG_S t0, (37 * EFEX_REGSZ)(sp)
	csrr t0, 0x7c2 /* mcor */
	REG_S t0, (38 * EFEX_REGSZ)(sp)
	csrr t0, 0x7c5 /* mhint */
	REG_S t0, (39 * EFEX_REGSZ)(sp)

	csrci mstatus, 8
	csrw mie, zero

	/* C907 C code may use the complete T-Head extension state. */
	li t0, MXSTATUS_C9XX
	csrs mxstatus, t0
	fence.i

	jal .Lclear_bss
	jal main
	REG_S a0, (9 * EFEX_REGSZ)(sp)
	jal syterkit_efex_arch_sync
	fence rw, rw
	fence.i
	.long 0x01b0000b /* sync.is */

	REG_L t0, (39 * EFEX_REGSZ)(sp)
	csrw 0x7c5, t0
	REG_L t0, (38 * EFEX_REGSZ)(sp)
	csrw 0x7c2, t0
	REG_L t0, (37 * EFEX_REGSZ)(sp)
	csrw 0x7c1, t0
	REG_L t0, (36 * EFEX_REGSZ)(sp)
	csrw 0x7c0, t0
	REG_L t0, (35 * EFEX_REGSZ)(sp)
	csrw satp, t0
	sfence.vma
	REG_L t0, (34 * EFEX_REGSZ)(sp)
	csrw 0x307, t0
	REG_L t0, (33 * EFEX_REGSZ)(sp)
	csrw mtvec, t0
	REG_L t0, (32 * EFEX_REGSZ)(sp)
	csrw mie, t0
	fence rw, rw
	fence.i
	.long 0x01b0000b /* sync.is */

	REG_L ra, (0 * EFEX_REGSZ)(sp)
	REG_L gp, (2 * EFEX_REGSZ)(sp)
	REG_L tp, (3 * EFEX_REGSZ)(sp)
	REG_L t0, (4 * EFEX_REGSZ)(sp)
	REG_L t1, (5 * EFEX_REGSZ)(sp)
	REG_L t2, (6 * EFEX_REGSZ)(sp)
	REG_L s0, (7 * EFEX_REGSZ)(sp)
	REG_L s1, (8 * EFEX_REGSZ)(sp)
	REG_L a0, (9 * EFEX_REGSZ)(sp)
	REG_L a1, (10 * EFEX_REGSZ)(sp)
	REG_L a2, (11 * EFEX_REGSZ)(sp)
	REG_L a3, (12 * EFEX_REGSZ)(sp)
	REG_L a4, (13 * EFEX_REGSZ)(sp)
	REG_L a5, (14 * EFEX_REGSZ)(sp)
	REG_L a6, (15 * EFEX_REGSZ)(sp)
	REG_L a7, (16 * EFEX_REGSZ)(sp)
	REG_L s2, (17 * EFEX_REGSZ)(sp)
	REG_L s3, (18 * EFEX_REGSZ)(sp)
	REG_L s4, (19 * EFEX_REGSZ)(sp)
	REG_L s5, (20 * EFEX_REGSZ)(sp)
	REG_L s6, (21 * EFEX_REGSZ)(sp)
	REG_L s7, (22 * EFEX_REGSZ)(sp)
	REG_L s8, (23 * EFEX_REGSZ)(sp)
	REG_L s9, (24 * EFEX_REGSZ)(sp)
	REG_L s10, (25 * EFEX_REGSZ)(sp)
	REG_L s11, (26 * EFEX_REGSZ)(sp)
	REG_L t3, (27 * EFEX_REGSZ)(sp)
	REG_L t4, (28 * EFEX_REGSZ)(sp)
	REG_L t5, (29 * EFEX_REGSZ)(sp)
	REG_L t6, (30 * EFEX_REGSZ)(sp)

	/* Restore mstatus last, then reload t0 and sp before returning. */
	REG_L t0, (31 * EFEX_REGSZ)(sp)
	csrw mstatus, t0
	REG_L t0, (4 * EFEX_REGSZ)(sp)
	REG_L sp, (1 * EFEX_REGSZ)(sp)
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
