/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file syterkit.lds.S
 * @brief ARM linker script for the SyterKit firmware image.
 *
 * Lays out the SPL output sections in RAM for the ARM32 target, with
 * separate load segments for text and data.
 */
OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)
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

#ifdef CONFIG_ARCH_EXCEPTION
#define IRQ_STACK_SIZE 0x100
#else
#define IRQ_STACK_SIZE 0
#endif

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

	.rodata : ALIGN(16) { *(.rodata .rodata.*) } > ram :text
	.data : ALIGN(16) { *(.data .data.*) } > ram :data
	.ARM.exidx : {
		__exidx_start = .;
		*(.ARM.exidx* .gnu.linkonce.armexidx.*)
		__exidx_end = .;
	} > ram :text

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
		. = ALIGN(16);
		__stack_und_start = .; . += IRQ_STACK_SIZE; __stack_und_end = .;
		__stack_abt_start = .; . += IRQ_STACK_SIZE; __stack_abt_end = .;
		__stack_irq_start = .; . += IRQ_STACK_SIZE; __stack_irq_end = .;
		__stack_fiq_start = .; . += IRQ_STACK_SIZE; __stack_fiq_end = .;
		__stack_srv_start = .; . += 0x1000; __stack_srv_end = .;
	} > ram :data

	. = ALIGN(4);
	_end = .;
	ASSERT(_start == __spl_start + 0x40,
	       "Boot0 entry must be 0x40 bytes from the image start")
}
