/* SPDX-License-Identifier: GPL-2.0+ */

OUTPUT_ARCH(aarch64)
ENTRY(_start)

SECTIONS
{
	. = 0;
	.text : { *(.text) }
	/DISCARD/ : { *(*) }
}
