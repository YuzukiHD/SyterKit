OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)
SECTIONS
{
	. = 0x4a000000;
	. = ALIGN(4);

	.text :
	{
        .obj/source/start.o (*.text)
		*(.text)
		KEEP(*(.note.gnu.build-id))
	}

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
