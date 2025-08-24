/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <config.h>
#include <log.h>

#include <mmu.h>
#include <common.h>
#include <jmp.h>

#include "sys-dram.h"
#include "sys-sdcard.h"
#include "sys-sid.h"
#include "sys-spi.h"

#include <cli.h>
#include <cli_shell.h>
#include <cli_termesc.h>

#include <image_loader.h>

#include "ff.h"
#include "libfdt.h"

#define CONFIG_KERNEL_FILENAME "zImage"
#define CONFIG_DTB_FILENAME "sunxi.dtb"

#define CONFIG_SDMMC_SPEED_TEST_SIZE 1024// (unit: 512B sectors)

#define CONFIG_DTB_LOAD_ADDR (0x41008000)
#define CONFIG_KERNEL_LOAD_ADDR (0x41800000)

// 128KB erase sectors, so place them starting from 2nd sector
#define CONFIG_SPINAND_DTB_ADDR (128 * 2048)
#define CONFIG_SPINAND_KERNEL_ADDR (256 * 2048)

#define FILENAME_MAX_LEN 64
typedef struct {
	unsigned int offset;
	unsigned int length;
	unsigned char *dest;

	unsigned int of_offset;
	unsigned char *of_dest;

	char filename[FILENAME_MAX_LEN];
	char of_filename[FILENAME_MAX_LEN];
} image_info_t;

extern sunxi_serial_t uart_dbg;

extern sunxi_spi_t sunxi_spi0;

extern sdhci_t sdhci0;

extern dram_para_t dram_para;

image_info_t image;

#define CHUNK_SIZE 0x20000

static int fatfs_loadimage(char *filename, BYTE *dest) {
	FIL file;
	UINT byte_to_read = CHUNK_SIZE;
	UINT byte_read;
	UINT total_read = 0;
	FRESULT fret;
	int ret;
	uint32_t start, time;

	fret = f_open(&file, filename, FA_OPEN_EXISTING | FA_READ);
	if (fret != FR_OK) {
		printk_error("FATFS: open, filename: [%s]: error %d\n", filename, fret);
		ret = -1;
		goto open_fail;
	}

	start = time_ms();

	do {
		byte_read = 0;
		fret = f_read(&file, (void *) (dest), byte_to_read, &byte_read);
		dest += byte_to_read;
		total_read += byte_read;
	} while (byte_read >= byte_to_read && fret == FR_OK);

	time = time_ms() - start + 1;

	if (fret != FR_OK) {
		printk_error("FATFS: read: error %d\n", fret);
		ret = -1;
		goto read_fail;
	}
	ret = 0;

read_fail:
	fret = f_close(&file);

	printk_debug("FATFS: read in %ums at %.2fMB/S\n", time, (f32) (total_read / time) / 1024.0f);

open_fail:
	return ret;
}

static int load_sdcard(image_info_t *image) {
	FATFS fs;
	FRESULT fret;
	int ret;
	uint32_t start;

	uint32_t test_time;
	start = time_ms();
	sdmmc_blk_read(&card0, (uint8_t *) (SDRAM_BASE), 0, CONFIG_SDMMC_SPEED_TEST_SIZE);
	test_time = time_ms() - start;
	printk_debug("SDMMC: speedtest %uKB in %ums at %uKB/S\n", (CONFIG_SDMMC_SPEED_TEST_SIZE * 512) / 1024, test_time, (CONFIG_SDMMC_SPEED_TEST_SIZE * 512) / test_time);

	start = time_ms();

	fret = f_mount(&fs, "", 1);
	if (fret != FR_OK) {
		printk_error("FATFS: mount error: %d\n", fret);
		return -1;
	} else {
		printk_debug("FATFS: mount OK\n");
	}

	printk_info("FATFS: read %s addr=%x\n", image->of_filename, (unsigned int) image->of_dest);
	ret = fatfs_loadimage(image->of_filename, image->of_dest);
	if (ret)
		return ret;

	printk_info("FATFS: read %s addr=%x\n", image->filename, (unsigned int) image->dest);
	ret = fatfs_loadimage(image->filename, image->dest);
	if (ret)
		return ret;

	/* umount fs */
	fret = f_mount(0, "", 0);
	if (fret != FR_OK) {
		printk_error("FATFS: unmount error %d\n", fret);
		return -1;
	} else {
		printk_debug("FATFS: unmount OK\n");
	}
	printk_debug("FATFS: done in %ums\n", time_ms() - start);

	return 0;
}

int load_spi_nand(sunxi_spi_t *spi, image_info_t *image) {
	linux_zimage_header_t *hdr;
	unsigned int size;
	uint64_t start, time;

	if (spi_nand_detect(spi) != 0)
		return -1;

	/* get dtb size and read */
	spi_nand_read(spi, image->of_dest, CONFIG_SPINAND_DTB_ADDR, (uint32_t) sizeof(struct fdt_header));
	if (fdt_check_header(image->of_dest)) {
		printk_error("SPI-NAND: DTB verification failed\n");
		return -1;
	}

	size = fdt_totalsize(image->of_dest);
	printk_debug("SPI-NAND: dt blob: Copy from 0x%08x to 0x%08lx size:0x%08x\n", CONFIG_SPINAND_DTB_ADDR, (uint32_t) image->of_dest, size);
	start = time_us();
	spi_nand_read(spi, image->of_dest, CONFIG_SPINAND_DTB_ADDR, (uint32_t) size);
	time = time_us() - start;
	printk_info("SPI-NAND: read dt blob of size %u at %.2fMB/S\n", size, (f32) (size / time));

	/* get kernel size and read */
	spi_nand_read(spi, image->dest, CONFIG_SPINAND_KERNEL_ADDR, (uint32_t) sizeof(linux_zimage_header_t));
	hdr = (linux_zimage_header_t *) image->dest;
	if (hdr->magic != LINUX_ZIMAGE_MAGIC) {
		printk_debug("SPI-NAND: zImage verification failed\n");
		return -1;
	}
	size = hdr->end - hdr->start;
	printk_debug("SPI-NAND: Image: Copy from 0x%08x to 0x%08lx size:0x%08x\n", CONFIG_SPINAND_KERNEL_ADDR, (uint32_t) image->dest, size);
	start = time_us();
	spi_nand_read(spi, image->dest, CONFIG_SPINAND_KERNEL_ADDR, (uint32_t) size);
	time = time_us() - start;
	printk_info("SPI-NAND: read Image of size %u at %.2fMB/S\n", size, (f32) (size / time));

	return 0;
}

msh_declare_command(bootargs);
msh_define_help(bootargs, "get/set bootargs for kernel",
				"Usage: bootargs set \"bootargs\" - set new bootargs for zImage\n"
				"       bootargs get            - get current bootargs\n");
int cmd_bootargs(int argc, const char **argv) {
	if (argc < 2) {
		uart_puts(cmd_bootargs_usage);
		return 0;
	}

	if (strncmp(argv[1], "set", 3) == 0) {
		if (argc != 3) {
			uart_puts(cmd_bootargs_usage);
			return 0;
		}
		/* Force image.of_dest to be a pointer to fdt_header structure */
		struct fdt_header *dtb_header = (struct fdt_header *) image.of_dest;

		int err = 0;

		/* Check if DTB header is valid */
		if ((err = fdt_check_header(dtb_header)) != 0) {
			printk_error("Invalid device tree blob: %s\n", fdt_strerror(err));
			return 0;
		}

		int len = 0;
		/* Get the offset of "/chosen" node */
		uint32_t bootargs_node = fdt_path_offset(image.of_dest, "/chosen");

		/* Get bootargs string */
		char *bootargs_str = (void *) fdt_getprop(image.of_dest, bootargs_node, "bootargs", &len);
		printk(LOG_LEVEL_MUTE, "DTB OLD bootargs = \"%s\"\n", bootargs_str);

		/* New bootargs string */
		char *new_bootargs_str = argv[2];
		printk(LOG_LEVEL_MUTE, "Now set bootargs to \"%s\"\n", new_bootargs_str);

		/* Modify bootargs string */
		err = fdt_setprop(image.of_dest, bootargs_node, "bootargs", new_bootargs_str, strlen(new_bootargs_str) + 1);

		if (err < 0) {
			printk_error("libfdt fdt_setprop() error: %s\n", fdt_strerror(err));
			abort();
		}

		/* Get updated bootargs string */
		char *updated_bootargs_str = (void *) fdt_getprop(image.of_dest, bootargs_node, "bootargs", &len);
		printk(LOG_LEVEL_MUTE, "DTB NEW bootargs = \"%s\"\n", updated_bootargs_str);
	} else if (strncmp(argv[1], "get", 3) == 0) {
		/* Force image.of_dest to be a pointer to fdt_header structure */
		struct fdt_header *dtb_header = (struct fdt_header *) image.of_dest;

		int err = 0;

		/* Check if DTB header is valid */
		if ((err = fdt_check_header(dtb_header)) != 0) {
			printk_error("Invalid device tree blob: %s\n", fdt_strerror(err));
			return 0;
		}

		int len = 0;
		/* Get the offset of "/chosen" node */
		uint32_t bootargs_node = fdt_path_offset(image.of_dest, "/chosen");

		/* Get bootargs string */
		char *bootargs_str = (void *) fdt_getprop(image.of_dest, bootargs_node, "bootargs", &len);
		printk(LOG_LEVEL_MUTE, "DTB bootargs = \"%s\"\n", bootargs_str);
	} else {
		uart_puts(cmd_bootargs_usage);
	}

	return 0;
}

msh_declare_command(reload);
msh_define_help(reload, "rescan TF Card and reload DTB, Kernel zImage", "Usage: reload\n");
int cmd_reload(int argc, const char **argv) {
	if (sdmmc_init(&card0, &sdhci0) != 0) {
		printk_error("SMHC: init failed\n");
		return 0;
	}

	if (load_sdcard(&image) != 0) {
		printk_error("SMHC: loading failed\n");
		return 0;
	}
	return 0;
}

msh_declare_command(boot);
msh_define_help(boot, "boot to linux", "Usage: boot\n");
int cmd_boot(int argc, const char **argv) {
	uint32_t entry_point = 0;
	void (*kernel_entry)(int zero, int arch, unsigned int params);

	if (zImage_loader((unsigned char *) image.dest, &entry_point)) {
		printk_error("boot setup failed\n");
		return 0;
	}

	printk_info("booting linux...\n");

	arm32_mmu_disable();
	printk_info("disable mmu ok...\n");
	arm32_dcache_disable();
	printk_info("disable dcache ok...\n");
	arm32_icache_disable();
	printk_info("disable icache ok...\n");
	arm32_interrupt_disable();
	printk_info("free interrupt ok...\n");
	enable_kernel_smp();
	printk_info("enable kernel smp ok...\n");

	printk_info("jump to kernel address: 0x%x\n", image.dest);

	kernel_entry = (void (*)(int, int, unsigned int)) entry_point;
	kernel_entry(0, ~0, (unsigned int) image.of_dest);

	// if kernel boot not success, jump to fel.
	jmp_to_fel();
	return 0;
}

const msh_command_entry commands[] = {
		msh_define_command(bootargs),
		msh_define_command(reload),
		msh_define_command(boot),
		msh_command_end,
};

int main(void) {
	sunxi_serial_init(&uart_dbg);

	show_banner();

	sunxi_clk_init();

	sunxi_dram_init(&dram_para);

	sunxi_clk_dump();

	memset(&image, 0, sizeof(image_info_t));

	image.of_dest = (uint8_t *) CONFIG_DTB_LOAD_ADDR;
	image.dest = (uint8_t *) CONFIG_KERNEL_LOAD_ADDR;

	strcpy(image.filename, CONFIG_KERNEL_FILENAME);
	strcpy(image.of_filename, CONFIG_DTB_FILENAME);

	if (sunxi_sdhci_init(&sdhci0) != 0) {
		printk_error("SMHC: %s controller init failed\n", sdhci0.name);
		goto _shell;
	} else {
		printk_info("SMHC: %s controller v%x initialized\n", sdhci0.name, sdhci0.reg->vers);
	}
	if (sdmmc_init(&card0, &sdhci0) != 0) {
		printk_warning("SMHC: init failed\n");
		goto _shell;
	}

	if (load_sdcard(&image) != 0) {
		printk_warning("SMHC: loading failed\n");
		goto _shell;
	}

_shell:
	syterkit_shell_attach(commands);

	return 0;
}
