/* SPDX-License-Identifier: GPL-2.0+ */

OUTPUT_ARCH(riscv)
ENTRY(_start)
PHDRS { text PT_LOAD FLAGS(5); data PT_LOAD FLAGS(6); }
SECTIONS
{
	. = 0x80000000;
	__spl_start = .;
	.text : ALIGN(16) { KEEP(*(.text.start)) *(.text .text.*) } :text
	.rodata : ALIGN(16) { *(.rodata .rodata.*) } :text
	. = ALIGN(4096);
	.data : ALIGN(16) { *(.data .data.*) } :data
	.bss (NOLOAD) : ALIGN(16) { *(.bss .bss.* COMMON) } :data
	__spl_end = .;
	.stack (NOLOAD) : ALIGN(16) {
		__stack_srv_start = .;
		. += 64K;
		__stack_srv_end = .;
	} :data
}
