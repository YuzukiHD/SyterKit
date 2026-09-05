/* SPDX-License-Identifier: GPL-2.0+ */

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
#include <drivers/remoteproc/remoteproc.h>
#include <drivers/mmc/sdcard.h>
#include <drivers/sid/sid.h>
#include <drivers/spi/spi.h>
#include <dt-compatible/dram-dt.h>
#include <dt-compatible/mmc-dt.h>
#include <dt-compatible/remoteproc-dt.h>

#include <lib/fatfs/ff.h>
#include <lib/fatfs/diskio.h>

#define CONFIG_SDMMC_SPEED_TEST_SIZE 1024 // (unit: 512B sectors)

extern sunxi_serial_t uart_dbg;

static sunxi_dram_t dram;

#define CHUNK_SIZE 0x20000

static int fatfs_loadimage(const char *filename, BYTE *dest)
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

static int load_sdcard(sunxi_remoteproc_t *remoteproc, sdmmc_pdata_t *card)
{
	FATFS fs;
	FRESULT fret;
	int ret;
	size_t index;
	uint32_t start;

	uint32_t test_time;
	start = time_ms();
	sdmmc_blk_read(card, (uint8_t *)(dram.memory_base), 0, CONFIG_SDMMC_SPEED_TEST_SIZE);
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

	for (index = 0U; index < remoteproc->firmware_count; ++index) {
		const sunxi_remoteproc_firmware_t *firmware = &remoteproc->firmware[index];

		pr_info("FATFS: read %s addr=%x\n", firmware->name, (unsigned int)firmware->load_address);
		ret = fatfs_loadimage(firmware->name, (BYTE *)firmware->load_address);
		if (ret)
			return ret;
	}

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
	sdmmc_pdata_t card = { 0 };
	sunxi_remoteproc_t c906;
	sunxi_sdhci_t sdhci0;

	if (sunxi_sdhci_dt_read_alias(&sdhci0, "mmc0") != DRIVER_OK || sunxi_remoteproc_dt_read_alias(&c906, "c906", NULL) != DRIVER_OK) {
		pr_err("Board: invalid devicetree configuration\n");
		return -1;
	}

	show_banner(); // Display a banner

	sunxi_clk_init(); // Initialize clock configurations

	if (sunxi_dram_dt_read_alias(&dram, "dram0") != DRIVER_OK) {
		pr_err("DRAM: invalid devicetree configuration\n");
		return -1;
	}
	sunxi_dram_init(&dram); // Initialize DRAM parameters

	sunxi_clk_dump(); // Dump clock information

	// Initialize SDHCI controller
	if (sunxi_sdhci_init(&sdhci0) != 0) {
		pr_err("SMHC: %s controller init failed\n", sdhci0.name);
		return 0;
	} else {
		pr_info("SMHC: %s controller initialized\n", sdhci0.name);
	}

	// Initialize SD/MMC card
	if (sdmmc_init(&card, &sdhci0) != 0) {
		pr_err("SMHC: init failed\n");
		return 0;
	}
	disk_set_device(0, &card);

	// Load image from SD card
	if (load_sdcard(&c906, &card) != 0) {
		pr_err("SMHC: loading failed\n");
		return 0;
	}

	if (sunxi_remoteproc_reset(&c906) != DRIVER_OK || sunxi_remoteproc_prepare(&c906) != DRIVER_OK || sunxi_remoteproc_load(&c906) != DRIVER_OK) {
		pr_err("RISC-V C906: prepare or load failed\n");
		return 0;
	}
	pr_info("RISC-V ELF run addr: 0x%08x\n", (uint32_t)c906.entry);

	pr_info("RISC-V C906 Core now Running... \n");

	mdelay(100); // Delay for 100 milliseconds

	if (sunxi_remoteproc_start(&c906) != DRIVER_OK) {
		pr_err("RISC-V C906: start failed\n");
		return 0;
	}

	abort(); // Abort A7 execution, loop forever

	jmp_to_fel(); // Jump to FEL mode

	return 0;
}
