/* SPDX-License-Identifier: GPL-2.0+ */

/*
 * SPI NOR boot path for Yuzuki Neko (sun252iw2 / F101 C907).
 *
 * This SPL is loaded by the BROM (FEL, SD/eMMC or SPI NOR eGON image) into
 * SRAM. It brings up PSRAM, then copies the boot payloads stored at fixed
 * offsets in the on-board SPI NOR flash into PSRAM and hands off to OpenSBI
 * (fw_jump), which in turn starts the Linux kernel.
 *
 * SPI NOR layout (all offsets are 2 KiB aligned):
 *	0x000000  bootloader   (this image, eGON BT0, up to 64 KiB)
 *	0x010000  device tree  (<= 256 KiB)
 *	0x050000  fw_jump.bin  (OpenSBI, 512 KiB)
 *	0x0d0000  Image        (Linux kernel, up to 6 MiB)
 *	0x6d0000  root filesystem
 */

#include <stdint.h>

#include <cache.h>
#include <cli/cli.h>
#include <cli/cli_shell.h>
#include <common.h>
#include <driver.h>
#include <log.h>
#include <malloc.h>

#include <drivers/clk/clk.h>
#include <drivers/mtd/spif-nor.h>
#include <drivers/psram/psram.h>
#include <drivers/serial/serial.h>
#include <drivers/spif/spif.h>
#include <dt-bindings/soc/sun252iw2.h>
#include <dt-compatible/psram-dt.h>
#include <dt-compatible/spif-dt.h>
#include <dt-compatible/spif-nor-dt.h>
#include <lib/fdt/libfdt.h>

/* PSRAM is the load target for every payload below. */
#define F101_RAM_BASE		SUNXI_PSRAM_BASE
#define F101_RAM_SIZE		0x01000000U

/*
 * PSRAM reservations, bottom up: Linux Image, then DTB, then OpenSBI pinned
 * at the top of PSRAM. The heap for the SPIF sampling training lives in the
 * gap between the kernel region and the DTB reservation so a kernel load can
 * never overwrite live malloc state.
 */
#define F101_KERNEL_SIZE	0x00600000U	/* Linux Image up to 6 MiB */
#define F101_DTB_SIZE		0x00040000U	/* Device tree up to 256 KiB */
#define F101_OPENSBI_SIZE	0x00080000U	/* OpenSBI fw_jump 512 KiB */

#define F101_LINUX_ADDR		(F101_RAM_BASE)
#define F101_OPENSBI_ADDR	(F101_RAM_BASE + F101_RAM_SIZE - F101_OPENSBI_SIZE)
#define F101_DTB_ADDR		(F101_OPENSBI_ADDR - F101_DTB_SIZE)

#define F101_HEAP_BASE		(F101_LINUX_ADDR + F101_KERNEL_SIZE)
#define F101_HEAP_SIZE		(F101_DTB_ADDR - F101_HEAP_BASE)

/*
 * Fixed SPI NOR layout. The bootloader itself is at most 64 KiB, so the
 * payloads start right after that slot and are packed back to back by their
 * maximum sizes (all offsets stay 2 KiB aligned).
 *	0x000000  bootloader  (this image, up to 64 KiB)
 *	0x010000  device tree (<= 256 KiB)
 *	0x050000  fw_jump.bin (OpenSBI, 512 KiB)
 *	0x0d0000  Image       (Linux kernel, up to 6 MiB)
 *	0x6d0000  root filesystem
 */
#define F101_BOOT_SIZE		0x00010000U

#define F101_NOR_DTB_OFFSET		F101_BOOT_SIZE
#define F101_NOR_OPENSBI_OFFSET		(F101_NOR_DTB_OFFSET + F101_DTB_SIZE)
#define F101_NOR_KERNEL_OFFSET		(F101_NOR_OPENSBI_OFFSET + F101_OPENSBI_SIZE)

typedef void (*opensbi_entry_t)(unsigned long hartid, uintptr_t fdt_addr);

static int f101_psram_init(void)
{
	sunxi_psram_t psram = { 0 };

	if (sunxi_psram_dt_read_alias(&psram, "psram0") != DRIVER_OK) {
		pr_err("PSRAM: invalid devicetree configuration\n");
		return -1;
	}

	if (sunxi_psram_init(&psram) == 0U) {
		pr_err("PSRAM: initialization failed\n");
		return -1;
	}

	pr_info("PSRAM: %u MiB initialized\n", sunxi_get_psram_size(&psram));
	return 0;
}

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

	/* Make images loaded from the SPI NOR visible to OpenSBI and Linux. */
	flush_dcache_all();
	asm volatile("fence rw, rw\n\tfence.i" ::: "memory");

	entry(0UL, F101_DTB_ADDR);
	__builtin_unreachable();
}

/*
 * Bring up the SPIF controller and detect the SPI NOR flash.
 */
static int f101_nor_setup(sunxi_spif_t *spif, spif_nor_t *nor)
{
	if (sunxi_spif_dt_read_alias(spif, "spif0") != DRIVER_OK) {
		pr_err("SPIF: invalid devicetree configuration\n");
		return -1;
	}

	if (spif_nor_dt_read_alias(nor, "spif-nor0", spif) != DRIVER_OK) {
		pr_err("SPIF NOR: invalid devicetree configuration\n");
		return -1;
	}

	if (sunxi_spif_init(spif) != 0) {
		pr_err("SPIF: controller init failed\n");
		return -1;
	}

	if (spif_nor_detect(nor) != 0) {
		pr_err("SPI NOR: no supported flash detected\n");
		return -1;
	}

	pr_info("SPI NOR: flash id 0x%08x, %u KiB\n", nor->info.id,
		nor->info.capacity >> 10);
	return 0;
}

/*
 * Read one fixed-size region from SPI NOR into PSRAM and report it.
 */
static int f101_read_from_nor(spif_nor_t *nor, const char *name, uint32_t offset,
			      uintptr_t dest, uint32_t size)
{
	uint32_t got = spif_nor_read(nor, (uint8_t *)dest, offset, size);

	if (got != size) {
		pr_err("SPI NOR: %s: short read at 0x%08x (wanted %u got %u)\n",
		       name, offset, size, got);
		return -1;
	}

	pr_info("SPI NOR: %-8s 0x%08x -> 0x%08x (%u KiB)\n", name, offset,
		(unsigned int)dest, size >> 10);
	return 0;
}

/*
 * Load the fixed-layout boot payloads from SPI NOR into PSRAM.
 */
static int f101_load_images(spif_nor_t *nor)
{
	if (f101_read_from_nor(nor, "dtb", F101_NOR_DTB_OFFSET,
			       F101_DTB_ADDR, F101_DTB_SIZE) != 0)
		return -1;

	if (f101_read_from_nor(nor, "fw_jump", F101_NOR_OPENSBI_OFFSET,
			       F101_OPENSBI_ADDR, F101_OPENSBI_SIZE) != 0)
		return -1;

	if (f101_read_from_nor(nor, "Image", F101_NOR_KERNEL_OFFSET,
			       F101_LINUX_ADDR, F101_KERNEL_SIZE) != 0)
		return -1;

	return 0;
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
	sunxi_spif_t spif = { 0 };
	spif_nor_t nor = { 0 };

	if (sunxi_serial_init_stdout() != DRIVER_OK)
		return -1;

	show_banner();
	sunxi_clk_init();

	if (f101_psram_init())
		return -1;

	/*
	 * The SPIF sampling training allocates temporary buffers with malloc;
	 * a failure only degrades reads to the safe 1-1-1 fallback timing.
	 */
	if (malloc_init(F101_HEAP_BASE, F101_HEAP_SIZE) != 0)
		pr_warn("SPI NOR: heap init failed, sampling training disabled\n");

	if (f101_nor_setup(&spif, &nor) != 0)
		goto fallback;

	if (f101_load_images(&nor) != 0)
		goto fallback;

	if (f101_validate_dtb() != 0)
		goto fallback;

	return cmd_boot(0, NULL);

fallback:
	pr_warn("SPI NOR boot failed; load images and run 'boot'\n");
	syterkit_shell_attach(commands);

	return cmd_boot(0, NULL);
}
