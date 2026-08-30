ENTRY(c907_boot0)

SECTIONS
{
	. = 0;

	.text : {
		KEEP(*(.text))
		*(.text.*)
	}

	/DISCARD/ : {
		*(.comment)
		*(.note*)
	}
}
