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
		. = ALIGN(64);
		KEEP(*(.text.entry))
		*(.text .text.*)
		PROVIDE(__ddr_bin_start = .);
		KEEP(*(.init_dram_bin))
		PROVIDE(__ddr_bin_end = .);
		KEEP(*(.note.gnu.build-id))
	} > ram :text
	.rodata : ALIGN(16) {
		*(.rdata .rdata.* .rodata .rodata.*)
		KEEP(*(.vectors))
	} > ram :text
	.data : ALIGN(16) { *(.data .data.* .sdata .sdata.*) } > ram :data

	PROVIDE(__spl_end = .);
	PROVIDE(__spl_size = __spl_end - __spl_start);
	PROVIDE(__code_start_address = SPL_TEXT_BASE);

	.bss (NOLOAD) : {
		. = ALIGN(4);
		_sbss = .;
		*(.sbss .sbss.* .bss .bss.* COMMON)
		. = ALIGN(4);
		_ebss = .;
	} > ram :data
	.stack (NOLOAD) : {
		. = ALIGN(16);
		__stack_srv_start = .; . += 0x1000; __stack_srv_end = .;
	} > ram :data
	. = ALIGN(4);
	_end = .;
#ifndef CONFIG_ARCH_BOOT
	ASSERT(_start == __spl_start + 0x40,
	       "Boot0 entry must be 0x40 bytes from the image start")
#else
	/*
	 * RV64 boards boot the C907 in RV32 mode: .boot0_head is boot_head
	 * (0x40) followed by the arch_boot payload arch_boot_payload (0x80), so
	 * the BROM's JUMP_INSTRUCTION (image + 0x40) hits the payload and the
	 * payload's PC-relative `la t1, _start` hits image + 0xc0.
	 */
	ASSERT(arch_boot_payload == __spl_start + 0x40,
	       "Boot0 entry (riscv_32_to_64) must be 0x40 bytes from the image start")
	ASSERT(_start == __spl_start + 0xc0,
	       "RV64 _start must be 0xc0 bytes from the image start")
#endif
}
