/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file efex.S
 * @brief Returning RISC-V eFEX entry for the Xuantie C906.
 *
 * eFEX is entered with the Boot ROM stack and must return to its caller.
 * Save and restore the integer register state without touching CSRs.  REGSZ,
 * SREG, and LREG come from linkage.h, so the frame layout follows the active
 * XLEN selected by the toolchain.
 */

#include <linkage.h>

/* x1..x31 plus one padding slot to keep the frame 16-byte aligned. */
#define EFEX_FRAME_SLOTS 32
#define EFEX_FRAME_SIZE  (EFEX_FRAME_SLOTS * REGSZ)

.macro EFEX_STORE reg, slot
	SREG \reg, (\slot * REGSZ)(sp)
.endm

.macro EFEX_LOAD reg, slot
	LREG \reg, (\slot * REGSZ)(sp)
.endm

	.section .text.entry, "ax", @progbits
	.balign 4
	.globl _start
	.type _start, @function
_start:
	/* Preserve the incoming stack and all integer registers. */
	addi	sp, sp, -EFEX_FRAME_SIZE
	EFEX_STORE ra, 0
	EFEX_STORE gp, 2
	EFEX_STORE tp, 3
	EFEX_STORE t0, 4
	EFEX_STORE t1, 5
	EFEX_STORE t2, 6
	EFEX_STORE s0, 7
	EFEX_STORE s1, 8
	EFEX_STORE a0, 9
	EFEX_STORE a1, 10
	EFEX_STORE a2, 11
	EFEX_STORE a3, 12
	EFEX_STORE a4, 13
	EFEX_STORE a5, 14
	EFEX_STORE a6, 15
	EFEX_STORE a7, 16
	EFEX_STORE s2, 17
	EFEX_STORE s3, 18
	EFEX_STORE s4, 19
	EFEX_STORE s5, 20
	EFEX_STORE s6, 21
	EFEX_STORE s7, 22
	EFEX_STORE s8, 23
	EFEX_STORE s9, 24
	EFEX_STORE s10, 25
	EFEX_STORE s11, 26
	EFEX_STORE t3, 27
	EFEX_STORE t4, 28
	EFEX_STORE t5, 29
	EFEX_STORE t6, 30
	addi	t0, sp, EFEX_FRAME_SIZE
	EFEX_STORE t0, 1

	fence.i

	jal	ra, .Lclear_bss
	jal	ra, main

	/* Preserve main's return value in the ABI a0 slot. */
	EFEX_STORE a0, 9
	jal	ra, syterkit_efex_arch_sync
	fence	rw, rw
	fence.i
	.long	0x01b0000b	/* sync.is */

	/* Restore the integer register set and return to the Boot ROM caller. */
	EFEX_LOAD ra, 0
	EFEX_LOAD gp, 2
	EFEX_LOAD tp, 3
	EFEX_LOAD t0, 4
	EFEX_LOAD t1, 5
	EFEX_LOAD t2, 6
	EFEX_LOAD s0, 7
	EFEX_LOAD s1, 8
	EFEX_LOAD a0, 9
	EFEX_LOAD a1, 10
	EFEX_LOAD a2, 11
	EFEX_LOAD a3, 12
	EFEX_LOAD a4, 13
	EFEX_LOAD a5, 14
	EFEX_LOAD a6, 15
	EFEX_LOAD a7, 16
	EFEX_LOAD s2, 17
	EFEX_LOAD s3, 18
	EFEX_LOAD s4, 19
	EFEX_LOAD s5, 20
	EFEX_LOAD s6, 21
	EFEX_LOAD s7, 22
	EFEX_LOAD s8, 23
	EFEX_LOAD s9, 24
	EFEX_LOAD s10, 25
	EFEX_LOAD s11, 26
	EFEX_LOAD t3, 27
	EFEX_LOAD t4, 28
	EFEX_LOAD t5, 29
	EFEX_LOAD t6, 30
	EFEX_LOAD sp, 1
	ret

/* Clear the linker-defined BSS range using the common 32-bit word size. */
.Lclear_bss:
	la	t0, _sbss
	la	t1, _ebss
	bgeu	t0, t1, .Lclear_bss_done
.Lclear_bss_loop:
	sw	zero, 0(t0)
	addi	t0, t0, 4
	bltu	t0, t1, .Lclear_bss_loop
.Lclear_bss_done:
	ret

	.size _start, . - _start
