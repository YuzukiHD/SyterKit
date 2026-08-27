# SPDX-License-Identifier: GPL-2.0+

config ARCH_BOOT
	bool "Boot through an arch boot payload"
	depends on ARCH_RISCV
	help
	  Boot the core through a small standalone payload instead of jumping
	  straight to _start.

	  Some RISC-V cores boot from the BROM in a reduced ISA mode and latch
	  their ISA at core reset, so the real entry point cannot be reached in
	  a single step.  The Xuantie C907, for example, starts in RV32 and only
	  becomes RV64 after a reset.  When enabled, the board embeds a payload
	  right after the boot0 head (in .boot0_head); the BROM's jump
	  instruction falls into it, the payload programs the post-reset start
	  address, switches the core mode, and resets the core into _start.

	  The payload is built independently of this Kbuild tree by the board's
	  arch_boot/payloads directory with the RV32 toolchain and emitted as
	  arch_boot_payload.inc:

	    make -C boards/<board>/arch_boot/payloads

	  Boards that need this mode switch should enable this option and
	  provide arch_boot/payloads/{arch_boot.S, arch_boot.lds, Makefile,
	  gen_payload.py}.
