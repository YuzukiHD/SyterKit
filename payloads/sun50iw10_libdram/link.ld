/* SPDX-License-Identifier: Apache-2.0 */

OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)
SEARCH_DIR(.)

/* Memory Spaces Definitions */
MEMORY
{
  ram   (rwx) : ORIGIN = 0x48000, LENGTH = 0x08000 /* SRAMC. */
}

/* Section Definitions */
SECTIONS
{
    .text :
    {
        . = ALIGN(4);
        PROVIDE(__spl_start = .);
        .obj/source/start.o (*.text)
        *(.text .text.*)
        KEEP(*(.note.gnu.build-id))
        . = ALIGN(4);
    } > ram

    . = ALIGN(4);

    .ARM.exidx : {
        __exidx_start = .;
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
        __exidx_end = .;
    } > ram

    PROVIDE(__spl_end = .);
    PROVIDE(__spl_size = __spl_end - __spl_start);

	. = ALIGN(16);
  	.rodata : 
    {
        *(.rodata) 
    }

	. = ALIGN(16);
  	.data : 
    { 
        *(.data) 
    }

	. = ALIGN(4);
	.bss :
	{
	    __bss_start = .;
		*(.bss)
	}

	. = ALIGN(4);
	__bss_end = .;

	_end = .;
}
