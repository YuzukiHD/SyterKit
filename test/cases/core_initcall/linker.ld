SECTIONS
{
	.initcalls : ALIGN(8)
	{
		__initcall_start = .;
		__initcallearly_start = .;
		KEEP(*(.initcallearly.init))
		__initcall1_start = .;
		KEEP(*(.initcall1.init))
		__initcall6_start = .;
		KEEP(*(.initcall6.init))
		__initcall7_start = .;
		KEEP(*(.initcall7.init))
		__initcall_end = .;
	}
}
INSERT AFTER .rodata;
