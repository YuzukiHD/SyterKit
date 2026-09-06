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

typedef void (*opensbi_entry_t)(unsigned long hartid, uintptr_t fdt_addr);

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

static __attribute__((noreturn)) void f101_boot_opensbi(void)
{
	opensbi_entry_t entry = (opensbi_entry_t)(uintptr_t)F101_OPENSBI_ADDR;

	/* Make the staged images visible to OpenSBI and Linux. */
	flush_dcache_all();
	asm volatile("fence rw, rw\n\tfence.i" ::: "memory");

	entry(0UL, F101_DTB_ADDR);
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
