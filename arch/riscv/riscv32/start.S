/* SPDX-License-Identifier: GPL-2.0+ */

#include <asm/trap.h>
#include <csr.h>

	.section .text.entry, "ax", @progbits
	.balign 4
	.globl _start
	.type _start, @function
_start:
	/* Boot ROM may leave machine interrupts enabled. */
	csrc mstatus, MSTATUS_MIE
	csrw mie, zero

	la sp, __stack_srv_end
	la a0, irq_handler
	ori a0, a0, 3
	csrw mtvec, a0
	jal clear_bss

	li t0, MSTATUS_FS | MSTATUS_XS
	csrs mstatus, t0

	/* Enable MXSTATUS.MM and the T-Head extended ISA. */
	li t0, MXSTATUS_MM | MXSTATUS_THEADISAEE
	csrs mxstatus, t0
	fence.i

	la a0, vectors
	csrw mtvt, a0

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
