/* SPDX-License-Identifier: Apache-2.0 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <types.h>

#include <log.h>
#include <config.h>

#include <common.h>
#include <arm32.h>
#include <jmp.h>

#include "sys-dram.h"
#include "sys-spi.h"
#include "sys-sdcard.h"
#include "sys-sid.h"

#include "ff.h"
#include "elf_loader.h"

extern uint32_t _start;
extern uint32_t __spl_start;
extern uint32_t __spl_end;
extern uint32_t __spl_size;
extern uint32_t __stack_srv_start;
extern uint32_t __stack_srv_end;
extern uint32_t __stack_ddr_srv_start;
extern uint32_t __stack_ddr_srv_end;

#define CONFIG_RISCV_ELF_FILENAME "e907.elf"
#define CONFIG_RISCV_ELF_LOADADDR (0x41008000)

#define CONFIG_SDMMC_SPEED_TEST_SIZE 1024 // (unit: 512B sectors)

sunxi_uart_t uart_dbg = {
	.base = 0x02500000,
	.id = 0,
	.gpio_tx = { GPIO_PIN(PORTH, 9), GPIO_PERIPH_MUX5 },
	.gpio_rx = { GPIO_PIN(PORTH, 10), GPIO_PERIPH_MUX5 },
};

sunxi_uart_t uart_e907 = {
	.base = 0x02500C00,
	.id = 3,
	.gpio_tx = { GPIO_PIN(PORTE, 0), GPIO_PERIPH_MUX7 },
	.gpio_rx = { GPIO_PIN(PORTE, 1), GPIO_PERIPH_MUX7 },
};

sdhci_t sdhci0 = {
	.name = "sdhci0",
	.reg = (sdhci_reg_t *)0x04020000,
	.voltage = MMC_VDD_27_36,
	.width = MMC_BUS_WIDTH_4,
	.clock = MMC_CLK_50M,
	.removable = 0,
	.isspi = FALSE,
	.gpio_clk = { GPIO_PIN(PORTF, 2), GPIO_PERIPH_MUX2 },
	.gpio_cmd = { GPIO_PIN(PORTF, 3), GPIO_PERIPH_MUX2 },
	.gpio_d0 = { GPIO_PIN(PORTF, 1), GPIO_PERIPH_MUX2 },
	.gpio_d1 = { GPIO_PIN(PORTF, 0), GPIO_PERIPH_MUX2 },
	.gpio_d2 = { GPIO_PIN(PORTF, 5), GPIO_PERIPH_MUX2 },
	.gpio_d3 = { GPIO_PIN(PORTF, 4), GPIO_PERIPH_MUX2 },
};

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
		printk(LOG_LEVEL_ERROR,
		       "FATFS: open, filename: [%s]: error %d\r\n", filename,
		       fret);
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
		printk(LOG_LEVEL_ERROR, "FATFS: read: error %d\r\n", fret);
		ret = -1;
		goto read_fail;
	}
	ret = 0;

read_fail:
	fret = f_close(&file);

	printk(LOG_LEVEL_DEBUG, "FATFS: read in %ums at %.2fMB/S\r\n", time,
	       (f32)(total_read / time) / 1024.0f);

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
	sdmmc_blk_read(&card0, (uint8_t *)(SDRAM_BASE), 0,
		       CONFIG_SDMMC_SPEED_TEST_SIZE);
	test_time = time_ms() - start;
	printk(LOG_LEVEL_DEBUG, "SDMMC: speedtest %uKB in %ums at %uKB/S\r\n",
	       (CONFIG_SDMMC_SPEED_TEST_SIZE * 512) / 1024, test_time,
	       (CONFIG_SDMMC_SPEED_TEST_SIZE * 512) / test_time);

	start = time_ms();

	fret = f_mount(&fs, "", 1);
	if (fret != FR_OK) {
		printk(LOG_LEVEL_ERROR, "FATFS: mount error: %d\r\n", fret);
		return -1;
	} else {
		printk(LOG_LEVEL_DEBUG, "FATFS: mount OK\r\n");
	}

	printk(LOG_LEVEL_INFO, "FATFS: read %s addr=%x\r\n", image->filename,
	       (unsigned int)image->dest);
	ret = fatfs_loadimage(image->filename, image->dest);
	if (ret)
		return ret;

	/* umount fs */
	fret = f_mount(0, "", 0);
	if (fret != FR_OK) {
		printk(LOG_LEVEL_ERROR, "FATFS: unmount error %d\r\n", fret);
		return -1;
	} else {
		printk(LOG_LEVEL_DEBUG, "FATFS: unmount OK\r\n");
	}
	printk(LOG_LEVEL_DEBUG, "FATFS: done in %ums\r\n", time_ms() - start);

	return 0;
}

void show_banner(void)
{
	uint32_t id[4];

	printk(LOG_LEVEL_MUTE, "\r\n");
	printk(LOG_LEVEL_INFO, " _____     _           _____ _ _   \r\n");
	printk(LOG_LEVEL_INFO, "|   __|_ _| |_ ___ ___|  |  |_| |_ \r\n");
	printk(LOG_LEVEL_INFO, "|__   | | |  _| -_|  _|    -| | _| \r\n");
	printk(LOG_LEVEL_INFO, "|_____|_  |_| |___|_| |__|__|_|_|  \r\n");
	printk(LOG_LEVEL_INFO, "      |___|                        \r\n");
	printk(LOG_LEVEL_INFO, "***********************************\r\n");
	printk(LOG_LEVEL_INFO, " %s V0.1.1 Commit: %s\r\n", PROJECT_NAME,
	       PROJECT_GIT_HASH);
	printk(LOG_LEVEL_INFO, "***********************************\r\n");

	id[0] = read32(0x03006200 + 0x0);
	id[1] = read32(0x03006200 + 0x4);
	id[2] = read32(0x03006200 + 0x8);
	id[3] = read32(0x03006200 + 0xc);

	printk(LOG_LEVEL_INFO, "Chip ID is: %08x%08x%08x%08x\r\n", id[0], id[1],
	       id[2], id[3]);
}

int main(void)
{
	sunxi_uart_init(&uart_dbg);

	// sunxi_uart_init(&uart_e907);

	show_banner();

	sunxi_clk_init();

	sunxi_dram_init();

	sunxi_clk_dump();

	memset(&image, 0, sizeof(image_info_t));

	image.dest = (uint8_t *)CONFIG_RISCV_ELF_LOADADDR;

	strcpy(image.filename, CONFIG_RISCV_ELF_FILENAME);

	if (sunxi_sdhci_init(&sdhci0) != 0) {
		printk(LOG_LEVEL_ERROR, "SMHC: %s controller init failed\r\n",
		       sdhci0.name);
		return 0;
	} else {
		printk(LOG_LEVEL_INFO,
		       "SMHC: %s controller v%x initialized\r\n", sdhci0.name,
		       sdhci0.reg->vers);
	}

	if (sdmmc_init(&card0, &sdhci0) != 0) {
		printk(LOG_LEVEL_ERROR, "SMHC: init failed\r\n");
		return 0;
	}

	if (load_sdcard(&image) != 0) {
		printk(LOG_LEVEL_ERROR, "SMHC: loading failed\r\n");
		return 0;
	}

	sunxi_e907_clock_reset();

	uint32_t elf_run_addr = elf_get_entry_addr((phys_addr_t)image.dest);
	printk(LOG_LEVEL_INFO, "RISC-V ELF run addr: 0x%08x\r\n", elf_run_addr);

	if (load_elf_image((phys_addr_t)image.dest)) {
		printk(LOG_LEVEL_ERROR, "RISC-V ELF load FAIL\r\n");
	}

	sunxi_e907_clock_init(elf_run_addr);

	dump_e907_clock();

	printk(LOG_LEVEL_INFO, "RISC-V E907 Core now Running... \r\n");

	abort();

	jmp_to_fel();

	return 0;
}