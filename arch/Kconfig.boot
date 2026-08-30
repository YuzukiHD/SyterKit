# SPDX-License-Identifier: GPL-2.0+

config ARCH_BOOT
	bool "Boot through an arch boot payload"
	depends on ARCH_RISCV
	help
	  Boot the core through a small standalone payload instead of jumping
	  straight to _start.

	  A board can use this when the BROM starts a helper core before the
	  target core, or when a core starts in a reduced ISA mode and latches
	  its final ISA at reset.  The board embeds arch_boot_payload directly
	  after the boot0 head; that payload prepares the target core and sends
	  it to the real _start entry point.

	  The payload may be a normal board assembly object when it uses the
	  configured ISA, or a byte array generated independently with the
	  helper core's toolchain.
