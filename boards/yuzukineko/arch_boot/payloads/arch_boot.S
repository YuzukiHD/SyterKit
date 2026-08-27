/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Switch the C907 core from RV32 to RV64.
 *
 * This is the arch_boot payload: it is compiled standalone by the payload
 * Makefile (not by the SyterKit Kbuild tree) into arch_boot_payload.inc, which
 * boards/yuzukineko/head.c embeds in the .boot0_head section right after
 * boot_head.  The BROM's JUMP_INSTRUCTION lands here (image + 0x40) in RV32;
 * _start -- the real RV64 entry, defined by the payload linker script at
 * payload + 0x80 -- follows right after.
 *
 * The C907 latches its ISA mode (RV32/RV64) at reset, so the mode change only
 * takes effect after the core is reset.  Sequence:
 *
 *   1. open the RISCV_CFG bus gate            (CCMU_RISCV_CFG_BGR_REG)
 *   2. program the post-reset start address  (_start, via PC-relative `la`)
 *   3. force RV64 mode                        (RISCV_CFG_BASE + 0x108)
 *   4. arm the RISC-V watchdog, which resets the core
 *   5. after the reset the core re-fetches from the start address, in RV64
 *
 * Addresses:
 *   CCU_RISCV_CFG_BGR_REG  = SUNXI_CCU_BASE + 0xd0c = 0x02001d0c
 *   RISCV_CFG_BASE          = 0x06010000
 *   RISCV_WDG_BASE          = 0x06011000
 */
	.section .text, "ax", @progbits
	.balign 4
	/* Force 4-byte encoding so the emitted bytes are stable across toolchains. */
	.option norvc
	.globl arch_boot
	.type arch_boot, @function
arch_boot:
	/* CCU_RISCV_CFG_BGR_REG <- 0x1 */
	li	t0, 0x02001d0c
	li	t1, 0x1
	sw	t1, 0(t0)

	/* start addr low32 <- address of RV64 _start (this stub + padding) */
	la	t1, _start
	li	t0, 0x06010100
	sw	t1, 0(t0)

	/* start addr high32 <- 0 */
	sw	zero, 4(t0)

	/* reset into RV64 mode */
	li	t1, 0x200
	sw	t1, 8(t0)

	/* irq disable */
	sw	zero, 0x6c(t0)

	/* RISCV_WDG_BASE <- 0x1 */
	li	t0, 0x06011000
	li	t1, 0x1
	sw	t1, 0(t0)

	/* wdog cfg  <- 0x16aa0000 */
	li	t1, 0x16aa0000
	sw	t1, 0x14(t0)

	/* wdog mode <- 0x16aa0011 (arm + trigger) */
	li	t1, 0x16aa0011
	sw	t1, 0x18(t0)

	/* wait for the reset */
1:	wfi
	j	1b
	.size arch_boot, . - arch_boot
