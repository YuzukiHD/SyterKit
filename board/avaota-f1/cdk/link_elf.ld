/* SPDX-License-Identifier: GPL-2.0+ */

OUTPUT_FORMAT("elf32-littleriscv", "elf32-littleriscv", "elf32-littleriscv")
OUTPUT_ARCH("riscv")
SEARCH_DIR(.)
ENTRY(_start)

/* Memory Spaces Definitions */
MEMORY
{
    ram (rwx) : ORIGIN = 0x02000000, LENGTH = 128K /* SRAM */
}

/* The stack size used by the application. NOTE: you need to adjust according to your application. */
STACK_SIZE = 0x1000; /* 4KB */

/* Section Definitions */
SECTIONS
{
    .text :
    {
        . = ALIGN(4);
        PROVIDE(__image_start = .);
        PROVIDE(__spl_start = .);
        KEEP(*(.boot0_head))
		KEEP(*start.o(*.text*))
		KEEP(*start.o(*.vectors*))
        *(.text .text.*)
        PROVIDE(__ddr_bin_start = .);
        KEEP(*(.init_dram_bin))
        PROVIDE(__ddr_bin_end = .);
        KEEP(*(.note.gnu.build-id))
        . = ALIGN(4);
    } > ram
	
    .rodata : 
    {
		. = ALIGN(4) ;
		*(.rdata)
		*(.rdata*)
		*(.rodata)
		*(.rodata*)
		. = ALIGN(4) ;
    } > ram

    .data : 
    {
		. = ALIGN(4) ;
		*(.data)
		*(.data*)
		*(.sdata*)
		. = ALIGN(4) ; 
	} > ram

    PROVIDE(__spl_end = .);
    PROVIDE(__spl_size = __spl_end - __spl_start);
    PROVIDE(__code_start_address = 0x02000000);

    /* .bss section which is used for uninitialized data */
    .bss (NOLOAD) :
    {
        . = ALIGN(4);
        _sbss = . ;
        *(.bss .bss.*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = . ;
	} > ram

    .stack (NOLOAD):
    {
        /* SRV stack section */
        __stack_srv_start = .;
        . += STACK_SIZE;
        __stack_srv_end = .;
    } > ram

    . = ALIGN(4);
    _end = . ;
}
