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
#include <drivers/i2c/i2c.h>
#include <drivers/remoteproc/remoteproc.h>
#include <drivers/rtc/rtc.h>
#include <drivers/mmc/sdcard.h>
#include <drivers/soc/sid.h>
#include <drivers/spi/spi.h>
#include <dt-compatible/dram-dt.h>
#include <dt-compatible/mmc-dt.h>
#include <dt-compatible/remoteproc-dt.h>

#include <cli/cli.h>
#include <cli/cli_shell.h>
#include <cli/cli_termesc.h>

#include <lib/fatfs/ff.h>
#include <lib/fatfs/diskio.h>

#define CONFIG_SDMMC_SPEED_TEST_SIZE 1024 // (unit: 512B sectors)

extern sunxi_serial_t uart_dbg;

static sunxi_dram_t dram;
static sunxi_remoteproc_t hifi4;

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
		printk_error("FATFS: open, filename: [%s]: error %d\n", filename, fret);
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
		printk_error("FATFS: read: error %d\n", fret);
		ret = -1;
		goto read_fail;
	}
	ret = 0;

read_fail:
	fret = f_close(&file);

	printk_debug("FATFS: read in %ums at %.2fMB/S\n", time, (f32)(total_read / time) / 1024.0f);

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
	printk_debug("SDMMC: speedtest %uKB in %ums at %uKB/S\n", (CONFIG_SDMMC_SPEED_TEST_SIZE * 512) / 1024, test_time, (CONFIG_SDMMC_SPEED_TEST_SIZE * 512) / test_time);

	start = time_ms();

	fret = f_mount(&fs, "", 1);
	if (fret != FR_OK) {
		printk_error("FATFS: mount error: %d\n", fret);
		return -1;
	} else {
		printk_debug("FATFS: mount OK\n");
	}

	for (index = 0U; index < remoteproc->firmware_count; ++index) {
		const sunxi_remoteproc_firmware_t *firmware = &remoteproc->firmware[index];

		printk_info("FATFS: read %s addr=%x\n", firmware->name, (unsigned int)firmware->load_address);
		ret = fatfs_loadimage(firmware->name, (BYTE *)firmware->load_address);
		if (ret)
			return ret;
	}

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

msh_declare_command(boot);
msh_define_help(boot, "boot to linux", "Usage: boot\n");
int cmd_boot(int argc, const char **argv)
{
	if (sunxi_remoteproc_start(&hifi4) != DRIVER_OK) {
		printk_error("HIFI4: start failed\n");
		return -1;
	}

	abort();
	return 0;
}

const msh_command_entry commands[] = {
	msh_define_command(boot),
	msh_command_end,
};

int main(void)
{
	sdmmc_pdata_t card = { 0 };
	sunxi_sdhci_t sdhci0;

	if (sunxi_sdhci_dt_read_alias(&sdhci0, "mmc0") != DRIVER_OK || sunxi_remoteproc_dt_read_alias(&hifi4, "hifi4", NULL) != DRIVER_OK) {
		printk_error("Board: invalid devicetree configuration\n");
		return -1;
	}

	show_banner(); // Display a banner

	sunxi_clk_init(); // Initialize clock configurations

	if (sunxi_dram_dt_read_alias(&dram, "dram0") != DRIVER_OK) {
		printk_error("DRAM: invalid devicetree configuration\n");
		return -1;
	}
	sunxi_dram_init(&dram);

	sunxi_clk_dump(); // Dump clock information

	// Initialize SDHCI controller
	if (sunxi_sdhci_init(&sdhci0) != 0) {
		printk_error("SMHC: %s controller init failed\n", sdhci0.name);
		return 0;
	} else {
		printk_info("SMHC: %s controller initialized\n", sdhci0.name);
	}

	// Initialize SD/MMC card
	if (sdmmc_init(&card, &sdhci0) != 0) {
		printk_error("SMHC: init failed\n");
		return 0;
	}
	disk_set_device(0, &card);

	// Load image from SD card
	if (load_sdcard(&hifi4, &card) != 0) {
		printk_error("SMHC: loading failed\n");
		return 0;
	}

	if (sunxi_remoteproc_reset(&hifi4) != DRIVER_OK || sunxi_remoteproc_prepare(&hifi4) != DRIVER_OK || sunxi_remoteproc_load(&hifi4) != DRIVER_OK) {
		printk_error("HIFI4: prepare or load failed\n");
		return 0;
	}
	printk_info("HIFI4 ELF run addr: 0x%08x\n", (uint32_t)hifi4.entry);

	printk_info("HIFI4 Core now Running... \n");

	cmd_boot(0, NULL);

	// if boot failed, attach the shell for debug
	syterkit_shell_attach(commands);

	jmp_to_fel(); // Jump to FEL mode

	return 0;
}
