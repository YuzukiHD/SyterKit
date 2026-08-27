/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Standalone link script for the RV32 bootstrap payload (arch_boot.S).
 *
 * The payload is assembled and linked once with the RV32 toolchain purely to
 * obtain its machine code; only the .text bytes are used, and the payload is
 * position independent (the `la t1, _start` inside it is PC-relative).
 *
 * The base address below mirrors the real load address (image + 0x40) so a
 * disassembly is easy to read.  The payload must be exactly 0x80 bytes so that,
 * with boot_head (0x40) + this payload, the image's ALIGN(64) puts the RV64
 * _start at image + 0xc0.  We pad .text to 0x80 here and define _start at
 * payload + 0x80 (what the `la` resolves against) so the source stays clean.
 */
ENTRY(arch_boot_payload)

SECTIONS
{
	. = 0x00020040;

	.text : {
		*(.text)
		/* pad to 0x80 so objcopy extracts exactly 0x80 bytes */
		. = arch_boot_payload + 0x80;
	}

	/* The RV64 entry point; the payload's PC-relative `la t1, _start`
	 * computes payload + 0x80 - (payload + 0x10) = 0x70. */
	_start = arch_boot_payload + 0x80;
}
