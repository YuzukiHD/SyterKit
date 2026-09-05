/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file start.S
 * @brief Xuantie C907 firmware start-up assembly.
 *
 * C907 code may use T-Head memory instructions in the early allocator, so
 * the complete extension state is enabled before any C code is called.
 */
#include <asm/trap.h>
#include <csr.h>

	.section .text.entry, "ax", @progbits
	.balign 4
	.globl _start
	.weak malloc_init_early
	.type _start, @function
_start:
	/* Clear L1 cache & BTB & BHT ... */
	li t0, 0x70013
	csrw mcor, t0

	/* Enable cache coherency */
	li t0, 0x1
	csrw msmpr, t0

	/* Boot ROM may leave machine interrupts enabled. */
	csrc mstatus, MSTATUS_MIE
	csrw mie, zero

	la sp, __stack_srv_end

#ifndef CONFIG_ARCH_EXCEPTION
	la a0, unhandled_exception
#else
	la a0, irq_handler
#if defined(CONFIG_ARCH_MTVT)
	ori a0, a0, 3
#endif
#endif
	csrw mtvec, a0

	li t0, MSTATUS_FS | MSTATUS_XS
	csrs mstatus, t0

	/* C907 DRAM/cache code uses the complete T-Head extension state. */
	li t0, MXSTATUS_C9XX
	csrs mxstatus, t0
	fence.i

	jal clear_bss

	/* Make the unused SRAM tail available when the allocator is linked. */
	la t0, malloc_init_early
	beqz t0, 1f
	jalr ra, t0, 0
1:

#if defined(CONFIG_ARCH_EXCEPTION) && defined(CONFIG_ARCH_MTVT)
	la a0, vectors
	csrw mtvt, a0
#endif

	jal set_timer_count

#ifdef CONFIG_ARCH_DCACHE
	jal cache_init

	jal icache_enable

	jal dcache_enable
#endif

	fence.i

	sync.is

	jal main

1:	wfi
	j 1b

clear_bss:
	la t0, _sbss
	la t1, _ebss
	bgeu t0, t1, 2f
1:	sw zero, 0(t0)
	addi t0, t0, 4
	bltu t0, t1, 1b
2:	ret

#ifdef CONFIG_ARCH_EXCEPTION
	.balign 64
	.globl irq_handler
	.type irq_handler, @function
irq_handler:
	SAVE_ALL
	jal riscv_handle_exception
	RESTORE_ALL

	.section .vectors, "a", @progbits
	.balign 64
	.globl vectors
	.type vectors, @object
vectors:
	.rept 16
	.word irq_handler
	.endr
	.size vectors, . - vectors
#else
	.balign 64
	.globl unhandled_exception
	.type unhandled_exception, @function
unhandled_exception:
	wfi
	j unhandled_exception
#endif
