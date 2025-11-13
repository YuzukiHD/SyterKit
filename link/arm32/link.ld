/* SPDX-License-Identifier: GPL-2.0+ */

OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)
SEARCH_DIR(.)
ENTRY(_start)

/* Memory Spaces Definitions */
MEMORY
{
  ram   (rwx) : ORIGIN = @ARCH_START_ADDRESS@, LENGTH = @ARCH_SRAM_LENGTH@ /* SRAMC. 132K on boot mode, 100K on FEL mode */
}

/* The stack size used by the application. NOTE: you need to adjust according to your application. */
MIN_STACK_SIZE = @MIN_STACK_SIZE@; /* 4B */
STACK_SIZE = 0x1000; /* 4KB */

/* Section Definitions */
SECTIONS
{
    . = ALIGN(4);
    .text :
    {
        PROVIDE(__spl_start = .);
        KEEP(*(.boot0_head))
        *(.text .text.*)
        PROVIDE(__ddr_bin_start = .);
        KEEP(*(.init_dram_bin))
        PROVIDE(__ddr_bin_end = .);
        KEEP(*(.note.gnu.build-id))
    } > ram

    . = ALIGN(16);
  	.rodata : 
    {
        *(.rodata) 
    } > ram

	. = ALIGN(16);
  	.data : 
    { 
        *(.data) 
    } > ram

    .ARM.exidx : {
        __exidx_start = .;
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
        __exidx_end = .;
    } > ram

    PROVIDE(__spl_end = .);
    PROVIDE(__spl_size = __spl_end - __spl_start);
    PROVIDE(__code_start_address = @ARCH_START_ADDRESS@);

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
        . = ALIGN(8);
        /* UND stack section */
        __stack_und_start = .;
        . += MIN_STACK_SIZE;
        __stack_und_end = .;
        /* ABT stack section */
        __stack_abt_start = .;
        . += MIN_STACK_SIZE;
        __stack_abt_end = .;
        /* IRQ stack section */
        __stack_irq_start = .;
        . += MIN_STACK_SIZE;
        __stack_irq_end = .;
        /* FIQ stack section */
        __stack_fiq_start = .;
        . += MIN_STACK_SIZE;
        __stack_fiq_end = .;
        /* SRV stack section */
        __stack_srv_start = .;
        . += STACK_SIZE;
        __stack_srv_end = .;
    } > ram

    . = ALIGN(4);
    _end = . ;
}
