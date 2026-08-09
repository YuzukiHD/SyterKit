/* SPDX-License-Identifier: GPL-2.0+ */

#ifdef CONFIG_ARCH_RISCV64
OUTPUT_FORMAT("elf64-littleriscv", "elf64-littleriscv", "elf64-littleriscv")
#else
OUTPUT_FORMAT("elf32-littleriscv", "elf32-littleriscv", "elf32-littleriscv")
#endif
OUTPUT_ARCH("riscv")
ENTRY(_start)

MEMORY
{
	ram (rwx) : ORIGIN = SPL_TEXT_BASE, LENGTH = SPL_MAX_SIZE
}

PHDRS
{
	text PT_LOAD FLAGS(5);
	data PT_LOAD FLAGS(6);
}

SECTIONS
{
	.text : {
		. = ALIGN(4);
		PROVIDE(__image_start = .);
		PROVIDE(__spl_start = .);
		KEEP(*(.boot0_head))
		*(.text .text.*)
		PROVIDE(__ddr_bin_start = .);
		KEEP(*(.init_dram_bin))
		PROVIDE(__ddr_bin_end = .);
		KEEP(*(.note.gnu.build-id))
	} > ram :text
	.rodata : ALIGN(16) { *(.rdata .rdata.* .rodata .rodata.*) } > ram :text
	.data : ALIGN(16) { *(.data .data.* .sdata .sdata.*) } > ram :data

	PROVIDE(__spl_end = .);
	PROVIDE(__spl_size = __spl_end - __spl_start);
	PROVIDE(__code_start_address = SPL_TEXT_BASE);

	.bss (NOLOAD) : {
		. = ALIGN(4);
		_sbss = .;
		*(.bss .bss.* COMMON)
		. = ALIGN(4);
		_ebss = .;
	} > ram :data
	.stack (NOLOAD) : {
		. = ALIGN(8);
		__stack_srv_start = .; . += 0x1000; __stack_srv_end = .;
	} > ram :data
	. = ALIGN(4);
	_end = .;
}
