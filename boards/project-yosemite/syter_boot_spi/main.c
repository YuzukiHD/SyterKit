/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <config.h>
#include <log.h>
#include <dt-compatible/ccu-dt.h>
#include <timer.h>

#include <common.h>
#include <jmp.h>
#include <mmu.h>

#include <cli/cli.h>
#include <cli/cli_shell.h>
#include <cli/cli_termesc.h>

#include <image/image_loader.h>

#include <drivers/dram.h>
#include <dt-compatible/rtc-dt.h>
#include <drivers/sid.h>
#include <drivers/spi.h>
#include <drivers/dma.h>
#include <drivers/mtd/spi-nand.h>
#include <dt-compatible/dma-dt.h>
#include <dt-compatible/dram-dt.h>
#include <dt-compatible/spi-nand-dt.h>
#include <dt-compatible/spi-dt.h>

#include <lib/fatfs/ff.h>
#include <lib/fdt/libfdt.h>
#include "uart.h"

#define CONFIG_KERNEL_FILENAME "zImage"
#define CONFIG_DTB_FILENAME "sunxi.dtb"

#define CONFIG_SDMMC_SPEED_TEST_SIZE 1024// (unit: 512B sectors)

#define CONFIG_DTB_LOAD_ADDR (0x41008000)
#define CONFIG_KERNEL_LOAD_ADDR (0x41800000)

// 128KB erase sectors, so place them starting from 2nd sector
#define CONFIG_SPINAND_DTB_ADDR (128 * 2048)
#define CONFIG_SPINAND_KERNEL_ADDR (256 * 2048)

#define CONFIG_DEFAULT_BOOTDELAY 5

#define FILENAME_MAX_LEN 64
typedef struct {
	uint32_t offset;
	uint32_t length;
	uint8_t *dest;

	uint32_t of_offset;
	uint8_t *of_dest;

	char filename[FILENAME_MAX_LEN];
	char of_filename[FILENAME_MAX_LEN];
} image_info_t;

extern sunxi_serial_t uart_dbg;


static sunxi_dram_t dram;

image_info_t image;

static int load_spi_nand(spi_nand_t *nand, image_info_t *image) {
	linux_zimage_header_t *hdr;
	unsigned int size;
	uint64_t start, time;

	if (spi_nand_detect(nand) != 0)
		return -1;

	/* get dtb size and read */
	spi_nand_read(nand, image->of_dest, CONFIG_SPINAND_DTB_ADDR, (uint32_t) sizeof(struct fdt_header));
	if (fdt_check_header(image->of_dest)) {
		printk_error("SPI-NAND: DTB verification failed\n");
		return -1;
	}

	size = fdt_totalsize(image->of_dest);
	printk_debug("SPI-NAND: dt blob: Copy from 0x%08x to 0x%08lx size:0x%08x\n", CONFIG_SPINAND_DTB_ADDR, (uint32_t) image->of_dest, size);
	start = time_us();
	spi_nand_read(nand, image->of_dest, CONFIG_SPINAND_DTB_ADDR, (uint32_t) size);
	time = time_us() - start;
	printk_info("SPI-NAND: read dt blob of size %u at %.2fMB/S\n", size, (f32) (size / time));

	/* get kernel size and read */
	spi_nand_read(nand, image->dest, CONFIG_SPINAND_KERNEL_ADDR, (uint32_t) sizeof(linux_zimage_header_t));
	hdr = (linux_zimage_header_t *) image->dest;
	if (hdr->magic != LINUX_ZIMAGE_MAGIC) {
		printk_debug("SPI-NAND: zImage verification failed\n");
		return -1;
	}
	size = hdr->end - hdr->start;
	printk_debug("SPI-NAND: Image: Copy from 0x%08x to 0x%08lx size:0x%08x\n", CONFIG_SPINAND_KERNEL_ADDR, (uint32_t) image->dest, size);
	start = time_us();
	spi_nand_read(nand, image->dest, CONFIG_SPINAND_KERNEL_ADDR, (uint32_t) size);
	time = time_us() - start;
	printk_info("SPI-NAND: read Image of size %u at %.2fMB/S\n", size, (f32) (size / time));

	return 0;
}

static int load_from_spi(image_info_t *image) {
	sunxi_dma_t dma;
	spi_nand_t nand;
	sunxi_spi_t spi;
	int result;

	if (sunxi_dma_dt_read_alias(&dma, "dma0") != DRIVER_OK ||
	    sunxi_spi_dt_read_alias(&spi, "spi0", &dma) != DRIVER_OK ||
	    spi_nand_dt_read_alias(&nand, "spi-nand0", &spi) != DRIVER_OK) {
		printk_error("SPI: invalid devicetree configuration\n");
		return -1;
	}
	if (sunxi_spi_init(&spi) != 0) {
		printk_error("SPI: init failed\n");
		return -1;
	}
	result = load_spi_nand(&nand, image);
	sunxi_spi_disable(&spi);
	return result;
}

static int abortboot_single_key(int bootdelay) {
	int abort = 0;
	unsigned long ts;

	printk_info("Hit any key to stop autoboot: %2d ", bootdelay);

	/* Check if key already pressed */
	if (tstc()) {		/* we got a key press */
		uart_getchar(); /* consume input */
		printk(LOG_LEVEL_MUTE, "\b\b\b%2d", bootdelay);
		abort = 1; /* don't auto boot */
	}

	while ((bootdelay > 0) && (!abort)) {
		--bootdelay;
		/* delay 1000 ms */
		ts = time_ms();
		do {
			if (tstc()) {  /* we got a key press */
				abort = 1; /* don't auto boot */
				break;
			}
			udelay(10000);
		} while (!abort && time_ms() - ts < 1000);
		printk(LOG_LEVEL_MUTE, "\b\b\b%2d ", bootdelay);
	}
	uart_putchar('\n');
	return abort;
}

msh_declare_command(reload);
msh_define_help(reload, "rescan SPI NAND and reload DTB, Kernel zImage", "Usage: reload\n");
int cmd_reload(int argc, const char **argv) {
	if (load_from_spi(&image) != 0) {
		printk_error("SPI-NAND: loading failed\n");
		return 0;
	}
	return 0;
}

msh_declare_command(boot);
msh_define_help(boot, "boot to linux", "Usage: boot\n");
int cmd_boot(int argc, const char **argv) {
	/* Initialize variables for kernel entry point and SD card access. */
	uint32_t entry_point = 0;
	void (*kernel_entry)(int zero, int arch, uint32_t params);

	/* Set up boot parameters for the kernel. */
	if (zImage_loader((uint8_t *) image.dest, &entry_point)) {
		printk_error("boot setup failed\n");
		abort();
	}

	/* Disable MMU, data cache, instruction cache, interrupts */
	clean_syterkit_data();

	enable_kernel_smp();
	printk_info("enable kernel smp ok...\n");

	/* Debug message to indicate the kernel address that the system is jumping to. */
	printk_info("jump to kernel address: 0x%x\n\n", image.dest);

	/* Jump to the kernel entry point. */
	kernel_entry = (void (*)(int, int, uint32_t)) entry_point;
	kernel_entry(0, ~0, (uint32_t) image.of_dest);

	// if kernel boot not success, jump to fel.
	jmp_to_fel();
	return 0;
}

const msh_command_entry commands[] = {
		msh_define_command(reload),
		msh_define_command(boot),
		msh_command_end,
};

int main(void) {
	sunxi_ccu_t ccu;
	sunxi_rtc_t rtc;

	/* Initialize the debug serial interface. */

	/* Display the bootloader banner. */
	show_banner();
	if (sunxi_rtc_dt_read_alias(&rtc, "rtc0") != DRIVER_OK) {
		printk_error("RTC: invalid devicetree configuration\n");
		return -1;
	}

	/* Initialize the system clock. */
	if (sunxi_ccu_dt_read(&ccu) != DRIVER_OK) {
		printk_error("CCU: invalid devicetree configuration\n");
		return -1;
	}

	sunxi_clk_init(&ccu);

	/* Check rtc fel flag. if set flag, goto fel */
	if (rtc_probe_fel_flag(&rtc)) {
		printk_info("RTC: get fel flag, jump to fel mode.\n");
		clean_syterkit_data();
		rtc_clear_fel_flag(&rtc);
		sunxi_clk_reset(&ccu);
		mdelay(100);
		goto _fel;
	}

	/* Initialize the DRAM and enable memory management unit (MMU). */
	if (sunxi_dram_dt_read_alias(&dram, "dram0", NULL, NULL) != DRIVER_OK) {
		printk_error("DRAM: invalid devicetree configuration\n");
		return -1;
	}
	uint32_t dram_size = sunxi_dram_init(&dram);
	arm32_mmu_enable(SDRAM_BASE, dram_size);

	/* Debug message to indicate that MMU is enabled. */
	printk_debug("enable mmu ok\n");

	/* Set up Real-Time Clock (RTC) hardware. */
	rtc_set_vccio_det_spare(&rtc);

	/* Check if system voltage is within limits. */
	sys_ldo_check(&ccu);

	/* Dump information about the system clocks. */
	sunxi_clk_dump(&ccu);

	/* Clear the image_info_t struct. */
	memset(&image, 0, sizeof(image_info_t));

	/* Set the destination address for the device tree binary (DTB), kernel image. */
	image.of_dest = (uint8_t *) CONFIG_DTB_LOAD_ADDR;
	image.dest = (uint8_t *) CONFIG_KERNEL_LOAD_ADDR;

	/* Copy the filenames for the DTB, kernel image. */
	strcpy(image.filename, CONFIG_KERNEL_FILENAME);
	strcpy(image.of_filename, CONFIG_DTB_FILENAME);

	/* Load the DTB, kernel image from the SPI NAND. */
	if (load_from_spi(&image) != 0) {
		printk_error("SPI-NAND: loading failed\n");
		goto _shell;
	}

	int bootdelay = CONFIG_DEFAULT_BOOTDELAY;

	/* Showing boot delays */
	if (abortboot_single_key(bootdelay)) {
		goto _shell;
	}

	cmd_boot(0, NULL);

_shell:
	syterkit_shell_attach(commands);

_fel:
	/* If the kernel boot fails, jump to FEL mode. */
	jmp_to_fel();

	/* Return 0 to indicate successful execution. */
	return 0;
}
