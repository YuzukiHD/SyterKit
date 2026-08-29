/* SPDX-License-Identifier: GPL-2.0+ */

#include <drivers/serial/serial.h>

#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <config.h>
#include <log.h>
#include <drivers/clk/clk.h>

#include <common.h>
#include <jmp.h>
#include <mmu.h>

#include <drivers/dram/dram.h>
#include <drivers/mmc/sdcard.h>
#include <drivers/sid/sid.h>
#include <drivers/spi/spi.h>
#include <dt-compatible/dram-dt.h>
#include <dt-compatible/mmc-dt.h>

#include "fdt_wrapper.h"
#include <lib/fatfs/ff.h>
#include <lib/fatfs/diskio.h>
#include <lib/fdt/libfdt.h>
#include <string.h>

#define CONFIG_DTB_FILENAME "sunxi.dtb"
#define CONFIG_DTB_LOADADDR (0x41008000)

#define CONFIG_SDMMC_SPEED_TEST_SIZE 1024 // (unit: 512B sectors)

static sunxi_dram_t dram;

static sunxi_sdhci_t sdhci0 = { 0 };
static sdmmc_pdata_t card0 = { 0 };

#define FILENAME_MAX_LEN 64
typedef struct {
	unsigned int offset;
	unsigned int length;
	unsigned char *dest;

	char filename[FILENAME_MAX_LEN];
} image_info_t;

image_info_t image;

#define CHUNK_SIZE 0x20000

static int fatfs_loadimage(char *filename, BYTE *dest)
{
	FIL file;
	UINT byte_to_read = CHUNK_SIZE;
	UINT byte_read;
	UINT total_read = 0;
	FRESULT fret;
	int ret;
	uint32_t start, time;

	fret = f_open(&file, filename, FA_OPEN_EXISTING | FA_READ);
	if (fret != FR_OK) {
		pr_err("FATFS: open, filename: [%s]: error %d\n", filename, fret);
		ret = -1;
		goto open_fail;
	}

	start = time_ms();

	do {
		byte_read = 0;
		fret = f_read(&file, (void *)(dest), byte_to_read, &byte_read);
		dest += byte_to_read;
		total_read += byte_read;
	} while (byte_read >= byte_to_read && fret == FR_OK);

	time = time_ms() - start + 1;

	if (fret != FR_OK) {
		pr_err("FATFS: read: error %d\n", fret);
		ret = -1;
		goto read_fail;
	}
	ret = 0;

read_fail:
	fret = f_close(&file);

	pr_debug("FATFS: read in %ums at %.2fMB/S\n", time, (f32)(total_read / time) / 1024.0f);

open_fail:
	return ret;
}

static int load_sdcard(image_info_t *image)
{
	FATFS fs;
	FRESULT fret;
	int ret;
	uint32_t start;

	uint32_t test_time;
	start = time_ms();
	sdmmc_blk_read(&card0, (uint8_t *)(dram.memory_base), 0, CONFIG_SDMMC_SPEED_TEST_SIZE);
	test_time = time_ms() - start;
	pr_debug("SDMMC: speedtest %uKB in %ums at %uKB/S\n", (CONFIG_SDMMC_SPEED_TEST_SIZE * 512) / 1024, test_time, (CONFIG_SDMMC_SPEED_TEST_SIZE * 512) / test_time);

	start = time_ms();

	fret = f_mount(&fs, "", 1);
	if (fret != FR_OK) {
		pr_err("FATFS: mount error: %d\n", fret);
		return -1;
	} else {
		pr_debug("FATFS: mount OK\n");
	}

	pr_info("FATFS: read %s addr=%x\n", image->filename, (unsigned int)image->dest);
	ret = fatfs_loadimage(image->filename, image->dest);
	if (ret)
		return ret;

	/* umount fs */
	fret = f_mount(0, "", 0);
	if (fret != FR_OK) {
		pr_err("FATFS: unmount error %d\n", fret);
		return -1;
	} else {
		pr_debug("FATFS: unmount OK\n");
	}
	pr_debug("FATFS: done in %ums\n", time_ms() - start);

	return 0;
}

int main(void)
{
	/* Initialize UART debug interface */

	/* Print boot screen */
	if (sunxi_serial_init_stdout() != 0)
		return -1;
	show_banner();
	if (sunxi_sdhci_dt_read_alias(&sdhci0, "mmc0") != DRIVER_OK) {
		pr_err("SMHC: invalid devicetree configuration\n");
		return -1;
	}

	/* Initialize clock */

	sunxi_clk_init();

	/* Initialize DRAM */
	if (sunxi_dram_dt_read_alias(&dram, "dram0") != DRIVER_OK) {
		pr_err("DRAM: invalid devicetree configuration\n");
		return -1;
	}
	sunxi_dram_init(&dram);

	/* Print clock information */
	sunxi_clk_dump();

	/* Clear image structure */
	memset(&image, 0, sizeof(image_info_t));

	/* Set the target address of image to DTB load address */
	image.dest = (uint8_t *)CONFIG_DTB_LOADADDR;

	/* Copy the DTB filename to the image structure */
	strcpy(image.filename, CONFIG_DTB_FILENAME);

	/* Initialize SD card controller */
	if (sunxi_sdhci_init(&sdhci0) != 0) {
		pr_err("SMHC: %s controller init failed\n", sdhci0.name);
		return 0;
	} else {
		pr_info("SMHC: %s controller initialized\n", sdhci0.name);
	}

	/* Initialize SD card */
	if (sdmmc_init(&card0, &sdhci0) != 0) {
		pr_err("SMHC: init failed\n");
		return 0;
	}
	disk_set_device(0, &card0);

	/* Load DTB file from SD card */
	if (load_sdcard(&image) != 0) {
		pr_err("SMHC: loading failed\n");
		return 0;
	}

	/* Force image.dest to be a pointer to fdt_header structure */
	struct fdt_header *dtb_header = (struct fdt_header *)image.dest;

	int err = 0;

	/* Check if DTB header is valid */
	if ((err = fdt_check_header(dtb_header)) != 0) {
		pr_err("Invalid device tree blob: %s\n", fdt_strerror(err));
		return -1;
	}

	/* Get the total size of DTB */
	uint32_t size = fdt_totalsize(image.dest);
	pr_info("DTB FDT Size = 0x%x\n", size);

	/* Print all device tree nodes */
	fdt_print(image.dest, "/", NULL, MAX_LEVEL);

	int len = 0;
	/* Get the offset of "/chosen" node */
	uint32_t bootargs_node = fdt_path_offset(image.dest, "/chosen");

	/* Get bootargs string */
	char *bootargs_str = (void *)fdt_getprop(image.dest, bootargs_node, "bootargs", &len);
	pr_info("DTB OLD bootargs = \"%s\"\n", bootargs_str);

	/* New bootargs string */
	char *new_bootargs_str = "earlyprintk=sunxi-uart,0x02500C00 root=/dev/mmcblk0p3 rootwait loglevel=8 initcall_debug=0 console=ttyS0 init=/init";
	pr_info("Now set bootargs to \"%s\"\n", new_bootargs_str);

	/* Modify bootargs string */
	err = fdt_setprop(image.dest, bootargs_node, "bootargs", new_bootargs_str, strlen(new_bootargs_str) + 1);

	if (err < 0) {
		pr_err("libfdt fdt_setprop() error: %s\n", fdt_strerror(err));
		abort();
	}

	/* Get updated bootargs string */
	char *updated_bootargs_str = (void *)fdt_getprop(image.dest, bootargs_node, "bootargs", &len);
	pr_info("DTB NEW bootargs = \"%s\"\n", updated_bootargs_str);

	/* Terminate program execution */
	abort();

	/* Jump to FEL mode execution */
	jmp_to_fel();

	return 0;
}
