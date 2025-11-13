/* SPDX-License-Identifier: GPL-2.0+ */

OUTPUT_FORMAT("elf64-littleriscv", "elf64-littleriscv", "elf64-littleriscv")
OUTPUT_ARCH("riscv")
SEARCH_DIR(.)
ENTRY(_start)

/* Memory Spaces Definitions */
MEMORY
{
  ram   (rwx) : ORIGIN = @ARCH_START_ADDRESS@, LENGTH = @ARCH_SRAM_LENGTH@ /* SRAMC. 132K on boot mode, 100K on FEL mode */
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
        *(.text .text.*)
        PROVIDE(__ddr_bin_start = .);
        KEEP(*(.init_dram_bin))
        PROVIDE(__ddr_bin_end = .);
        KEEP(*(.note.gnu.build-id))
        . = ALIGN(4);
    } > ram

	. = ALIGN(16);
  	.rodata : { *(.rodata) }

	. = ALIGN(16);
  	.data : { *(.data) }

    . = ALIGN(4);

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
        /* SRV stack section */
        __stack_srv_start = .;
        . += STACK_SIZE;
        __stack_srv_end = .;
    } > ram

    . = ALIGN(4);
    _end = . ;
}
