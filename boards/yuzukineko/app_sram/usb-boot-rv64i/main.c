/* SPDX-License-Identifier: GPL-2.0+ */

/*
 * USB/FEL boot path for Yuzuki Neko (sun252iw2 / F101 C907).
 *
 * This application is the in-memory sibling of spinor-boot: instead of
 * reading the boot payloads from SPI NOR, it expects the raw Linux Image,
 * DTB and OpenSBI fw_jump.bin to have already been staged in PSRAM by an
 * external loader (for example over USB/FEL with xfel). It only validates
 * and boots what is already there.
 *
 * The PSRAM memory map below is identical to spinor-boot so a host tool can
 * place images at the same addresses for either application.
 *
 * PSRAM layout (16 MiB PSRAM at SUNXI_PSRAM_BASE):
 *	0x40000000  Image        (Linux kernel, up to 6 MiB)
 *	0x40f40000  device tree  (<= 256 KiB)
 *	0x40f80000  fw_jump.bin  (OpenSBI, 512 KiB)
 */

#include <stdint.h>

#include <cache.h>
#include <cli/cli.h>
#include <cli/cli_shell.h>
#include <common.h>
#include <driver.h>
#include <log.h>

#include <drivers/clk/clk.h>
#include <drivers/serial/serial.h>
#include <dt-bindings/soc/sun252iw2.h>
#include <lib/fdt/libfdt.h>

#define F101_RAM_BASE		SUNXI_PSRAM_BASE
#define F101_RAM_SIZE		0x01000000U

#define F101_KERNEL_SIZE	0x00600000U	/* Linux Image up to 6 MiB */
#define F101_DTB_SIZE		0x00040000U	/* Device tree up to 256 KiB */
#define F101_OPENSBI_SIZE	0x00080000U	/* OpenSBI fw_jump 512 KiB */

#define F101_LINUX_ADDR		(F101_RAM_BASE)
#define F101_OPENSBI_ADDR	(F101_RAM_BASE + F101_RAM_SIZE - F101_OPENSBI_SIZE)
#define F101_DTB_ADDR		(F101_OPENSBI_ADDR - F101_DTB_SIZE)

/*
 * This entry is executed after the C907 has been reset into RV64 mode. It
 * must stay as raw instructions because this application itself is linked as
 * RV32. The reset vector points here, then the stub supplies the OpenSBI
 * arguments before jumping to fw_jump in PSRAM.
 *
 *	0x00000513  addi a0, zero, 0
 *	0x40f405b7  lui  a1, 0x40f40       (F101_DTB_ADDR)
 *	0x40f802b7  lui  t0, 0x40f80       (F101_OPENSBI_ADDR)
 *	0x0000100f  fence.i
 *	0x00028067  jr   t0
 */
static const uint32_t f101_rv64_entry[] __attribute__((section(".text.rv64_entry"), aligned(4), used)) = {
	0x00000513U,
	0x40f405b7U,
	0x40f802b7U,
	0x0000100fU,
	0x00028067U,
};

static int f101_validate_dtb(void)
{
	const void *fdt = (const void *)(uintptr_t)F101_DTB_ADDR;
	uint32_t size;
	int rc;

	rc = fdt_check_header(fdt);
	if (rc) {
		pr_err("DTB: invalid blob at 0x%08x: %s\n", F101_DTB_ADDR,
		       fdt_strerror(rc));
		return -1;
	}

	size = fdt_totalsize(fdt);
	if (size > F101_DTB_SIZE) {
		pr_err("DTB: %u bytes exceeds reserved %u bytes\n", size,
		       F101_DTB_SIZE);
		return -1;
	}

	return 0;
}

static __attribute__((noreturn, noinline)) void f101_boot_opensbi(void)
{
	uintptr_t rv64_entry = (uintptr_t)f101_rv64_entry;

	/* Make the staged images and the RV64 entry visible before the reset. */
	flush_dcache_all();
	asm volatile("fence rw, rw\n\tfence.i" ::: "memory");

	/*
	 * C907 latches the ISA mode at reset. Program the reset vector and force
	 * RV64, then use the RISC-V watchdog to restart the core. The reset lands
	 * in f101_rv64_entry, which sets a0/a1 and jumps to OpenSBI.
	 */
	asm volatile(
		"li t0, 0x02001d0c\n\t"
		"li t1, 0x1\n\t"
		"sw t1, 0(t0)\n\t"
		"li t0, 0x06010100\n\t"
		"sw %[entry], 0(t0)\n\t"
		"sw zero, 4(t0)\n\t"
		"li t1, 0x200\n\t"
		"sw t1, 8(t0)\n\t"
		"sw zero, 0x6c(t0)\n\t"
		"li t0, 0x06011000\n\t"
		"li t1, 0x1\n\t"
		"sw t1, 0(t0)\n\t"
		"li t1, 0x16aa0000\n\t"
		"sw t1, 0x14(t0)\n\t"
		"li t1, 0x16aa0011\n\t"
		"sw t1, 0x18(t0)\n"
		"1: wfi\n\t"
		"j 1b\n"
		:
		: [entry] "r" ((uint32_t)rv64_entry)
		: "t0", "t1", "memory");

	__builtin_unreachable();
}

msh_declare_command(boot);
msh_define_help(boot, "boot images already loaded in memory", "Usage: boot");

int cmd_boot(int argc, const char **argv)
{
	(void)argc;
	(void)argv;

	if (f101_validate_dtb())
		return -1;

	pr_info("Booting memory images: Linux=0x%08x DTB=0x%08x OpenSBI=0x%08x\n",
		F101_LINUX_ADDR, F101_DTB_ADDR, F101_OPENSBI_ADDR);
	f101_boot_opensbi();
}

static const msh_command_entry commands[] = {
	msh_define_command(boot),
	msh_command_end,
};

int main(void)
{
	if (sunxi_serial_init_stdout() != DRIVER_OK)
		return -1;

	show_banner();
	sunxi_clk_init();

	/*
	 * PSRAM is expected to already be initialized and to hold the staged
	 * images (loaded over USB/FEL by the host). Re-running the PSRAM
	 * bring-up here could disturb that content, so it stays disabled; see
	 * spinor-boot's f101_psram_init() if this app ever needs to do it.
	 */
	if (f101_validate_dtb()) {
		pr_warn("Memory images are not ready; load them and run 'boot'\n");
		syterkit_shell_attach(commands);
	}

	return cmd_boot(0, NULL);
}
