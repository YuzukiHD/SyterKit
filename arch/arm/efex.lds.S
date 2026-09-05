/* SPDX-License-Identifier: GPL-2.0+ */

OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)

MEMORY { ram (rwx) : ORIGIN = SPL_TEXT_BASE, LENGTH = SPL_MAX_SIZE }
PHDRS { text PT_LOAD FLAGS(5); data PT_LOAD FLAGS(6); }

SECTIONS
{
	.text : {
		PROVIDE(__spl_start = .);
		KEEP(*(.boot0_head))
		. = ALIGN(64);
		KEEP(*(.text.entry))
		*(.text .text.*)
		PROVIDE(__ddr_bin_start = .);
		KEEP(*(.init_dram_bin))
		PROVIDE(__ddr_bin_end = .);
		KEEP(*(.note.gnu.build-id))
	} > ram :text

	.rodata : ALIGN(16) {
		*(.rodata .rodata.*)
	} > ram :text

	.data : ALIGN(16) {
		*(.data .data.*)
	} > ram :data

	.efex_result : ALIGN(16) {
		PROVIDE(__efex_result_start = .);
		KEEP(*(.efex.result))
		PROVIDE(__efex_result_end = .);
	} > ram :data

	.ARM.exidx : {
		__exidx_start = .;
		*(.ARM.exidx* .gnu.linkonce.armexidx.*)
		__exidx_end = .;
	} > ram :text

	PROVIDE(__spl_end = .);
	PROVIDE(__spl_size = __spl_end - __spl_start);
	PROVIDE(__code_start_address = SPL_TEXT_BASE);

	.bss (NOLOAD) : {
		. = ALIGN(4); _sbss = .;
		*(.bss .bss.* COMMON)
		. = ALIGN(4); _ebss = .;
	} > ram :data

	. = ALIGN(4); _end = .;
	PROVIDE(__sram_end = ORIGIN(ram) + LENGTH(ram));
	ASSERT(_start == __spl_start + 0x40,
	       "eFEX entry must be 0x40 bytes from the image start")
}
