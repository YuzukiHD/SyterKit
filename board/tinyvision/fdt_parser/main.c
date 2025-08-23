/* SPDX-License-Identifier: GPL-2.0+ */

#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <config.h>
#include <log.h>

#include <common.h>
#include <jmp.h>
#include <mmu.h>

#include "sys-dram.h"
#include "sys-sdcard.h"
#include "sys-sid.h"
#include "sys-spi.h"

#include "fdt_wrapper.h"
#include "ff.h"
#include "libfdt.h"

#define CONFIG_DTB_FILENAME "sunxi.dtb"
#define CONFIG_DTB_LOADADDR (0x41008000)

#define CONFIG_SDMMC_SPEED_TEST_SIZE 1024// (unit: 512B sectors)

extern sunxi_serial_t uart_dbg;

extern dram_para_t dram_para;

sunxi_serial_t uart_e907 = {
		.base = 0x02500C00,
		.id = 3,
		.baud_rate = UART_BAUDRATE_115200,
		.dlen = UART_DLEN_8,
		.stop = UART_STOP_BIT_0,
		.parity = UART_PARITY_NO,
		.gpio_pin =
				{
						.gpio_tx = {GPIO_PIN(GPIO_PORTE, 0), GPIO_PERIPH_MUX7},
						.gpio_rx = {GPIO_PIN(GPIO_PORTE, 1), GPIO_PERIPH_MUX7},
				},
		.uart_clk =
				{
						.gate_reg_base = CCU_BASE + CCU_UART_BGR_REG,
						.gate_reg_offset = SERIAL_DEFAULT_CLK_GATE_OFFSET(3),
						.rst_reg_base = CCU_BASE + CCU_UART_BGR_REG,
						.rst_reg_offset = SERIAL_DEFAULT_CLK_RST_OFFSET(3),
						.parent_clk = SERIAL_DEFAULT_PARENT_CLK,
				},
};

extern sdhci_t sdhci0;

#define FILENAME_MAX_LEN 64
typedef struct {
	unsigned int offset;
	unsigned int length;
	unsigned char *dest;

	char filename[FILENAME_MAX_LEN];
} image_info_t;

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


int main(void) {
	/* Initialize UART debug interface */
	sunxi_serial_init(&uart_dbg);

	/* Print boot screen */
	show_banner();

	/* Initialize clock */
	sunxi_clk_init();

	/* Initialize DRAM */
	sunxi_dram_init(&dram_para);

	/* Print clock information */
	sunxi_clk_dump();

	/* Clear image structure */
	memset(&image, 0, sizeof(image_info_t));

	/* Set the target address of image to DTB load address */
	image.dest = (uint8_t *) CONFIG_DTB_LOADADDR;

	/* Copy the DTB filename to the image structure */
	strcpy(image.filename, CONFIG_DTB_FILENAME);

	/* Initialize SD card controller */
	if (sunxi_sdhci_init(&sdhci0) != 0) {
		printk_error("SMHC: %s controller init failed\n", sdhci0.name);
		return 0;
	} else {
		printk_info("SMHC: %s controller v%x initialized\n", sdhci0.name, sdhci0.reg->vers);
	}

	/* Initialize SD card */
	if (sdmmc_init(&card0, &sdhci0) != 0) {
		printk_error("SMHC: init failed\n");
		return 0;
	}

	/* Load DTB file from SD card */
	if (load_sdcard(&image) != 0) {
		printk_error("SMHC: loading failed\n");
		return 0;
	}

	/* Force image.dest to be a pointer to fdt_header structure */
	struct fdt_header *dtb_header = (struct fdt_header *) image.dest;

	int err = 0;

	/* Check if DTB header is valid */
	if ((err = fdt_check_header(dtb_header)) != 0) {
		printk_error("Invalid device tree blob: %s\n", fdt_strerror(err));
		return -1;
	}

	/* Get the total size of DTB */
	uint32_t size = fdt_totalsize(image.dest);
	printk_info("DTB FDT Size = 0x%x\n", size);

	/* Print all device tree nodes */
	fdt_print(image.dest, "/", NULL, MAX_LEVEL);

	int len = 0;
	/* Get the offset of "/chosen" node */
	uint32_t bootargs_node = fdt_path_offset(image.dest, "/chosen");

	/* Get bootargs string */
	char *bootargs_str = (void *) fdt_getprop(image.dest, bootargs_node, "bootargs", &len);
	printk_info("DTB OLD bootargs = \"%s\"\n", bootargs_str);

	/* New bootargs string */
	char *new_bootargs_str = "earlyprintk=sunxi-uart,0x02500C00 root=/dev/mmcblk0p3 rootwait loglevel=8 initcall_debug=0 console=ttyS0 init=/init";
	printk_info("Now set bootargs to \"%s\"\n", new_bootargs_str);

	/* Modify bootargs string */
	err = fdt_setprop(image.dest, bootargs_node, "bootargs", new_bootargs_str, strlen(new_bootargs_str) + 1);

	if (err < 0) {
		printk_error("libfdt fdt_setprop() error: %s\n", fdt_strerror(err));
		abort();
	}

	/* Get updated bootargs string */
	char *updated_bootargs_str = (void *) fdt_getprop(image.dest, bootargs_node, "bootargs", &len);
	printk_info("DTB NEW bootargs = \"%s\"\n", updated_bootargs_str);

	/* Terminate program execution */
	abort();

	/* Jump to FEL mode execution */
	jmp_to_fel();

	return 0;
}
