/* SPDX-License-Identifier: GPL-2.0+ */

#include <drivers/serial/serial.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <config.h>
#include <log.h>
#include <drivers/clk/clk.h>

#include <mmu.h>
#include <common.h>
#include <jmp.h>

#include <image/image_loader.h>

#include <drivers/dram/dram.h>
#include <drivers/remoteproc/remoteproc.h>
#include <drivers/mmc/sdcard.h>
#include <drivers/sid/sid.h>
#include <drivers/spi/spi.h>
#include <dt-compatible/dram-dt.h>
#include <dt-compatible/mmc-dt.h>
#include <dt-compatible/remoteproc-dt.h>
#include <dt-compatible/serial-dt.h>

#include <lib/fdt/libfdt.h>
#include <lib/fatfs/ff.h>
#include <lib/fatfs/diskio.h>
#include <string.h>

#define CONFIG_KERNEL_FILENAME "zImage"
#define CONFIG_DTB_FILENAME "sunxi.dtb"

#define CONFIG_SDMMC_SPEED_TEST_SIZE 1024 // (unit: 512B sectors)

#define CONFIG_DTB_LOAD_ADDR (0x41008000)
#define CONFIG_KERNEL_LOAD_ADDR (0x41800000)

// 128KB erase sectors, so place them starting from 2nd sector
#define CONFIG_SPINAND_DTB_ADDR (128 * 2048)
#define CONFIG_SPINAND_KERNEL_ADDR (256 * 2048)

#define FILENAME_MAX_LEN 64
typedef struct {
	unsigned int offset;
	unsigned char *dest;

	unsigned int of_offset;
	unsigned char *of_dest;

	char filename[FILENAME_MAX_LEN];
	char of_filename[FILENAME_MAX_LEN];
} image_info_t;

static sunxi_dram_t dram;

static sunxi_sdhci_t sdhci0 = { 0 };
static sdmmc_pdata_t card0 = { 0 };

image_info_t image;

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

static int load_sdcard(image_info_t *image, sunxi_remoteproc_t *remoteproc)
{
	FATFS fs;
	FRESULT fret;
	int ret;
	size_t index;
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

	pr_info("FATFS: read %s addr=%x\n", image->of_filename, (unsigned int)image->of_dest);
	ret = fatfs_loadimage(image->of_filename, image->of_dest);
	if (ret)
		return ret;

	pr_info("FATFS: read %s addr=%x\n", image->filename, (unsigned int)image->dest);
	ret = fatfs_loadimage(image->filename, image->dest);
	if (ret)
		return ret;

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
	sunxi_remoteproc_t e907;
	sunxi_serial_t uart_e907;

	if (sunxi_sdhci_dt_read_alias(&sdhci0, "mmc0") != DRIVER_OK) {
		pr_err("SMHC: invalid devicetree configuration\n");
		return -1;
	}
	if (sunxi_serial_dt_read_alias(&uart_e907, "uart-e907") != DRIVER_OK || sunxi_remoteproc_dt_read_alias(&e907, "e907", NULL) != DRIVER_OK) {
		pr_err("Board: invalid devicetree configuration\n");
		return -1;
	}

	sunxi_serial_init(&uart_e907);

	if (sunxi_serial_init_stdout() != 0)
		return -1;

	show_banner();

	sunxi_clk_init();

	if (sunxi_dram_dt_read_alias(&dram, "dram0") != DRIVER_OK) {
		pr_err("DRAM: invalid devicetree configuration\n");
		return -1;
	}
	sunxi_dram_init(&dram);

	uint32_t entry_point = 0;
	void (*kernel_entry)(int zero, int arch, unsigned int params);

	sunxi_clk_dump();

	memset(&image, 0, sizeof(image_info_t));

	image.of_dest = (uint8_t *)CONFIG_DTB_LOAD_ADDR;
	image.dest = (uint8_t *)CONFIG_KERNEL_LOAD_ADDR;

	strcpy(image.filename, CONFIG_KERNEL_FILENAME);
	strcpy(image.of_filename, CONFIG_DTB_FILENAME);

	if (sunxi_sdhci_init(&sdhci0) != 0) {
		pr_err("SMHC: %s controller init failed\n", sdhci0.name);
	} else {
		pr_info("SMHC: %s controller initialized\n", sdhci0.name);
	}
	if (sdmmc_init(&card0, &sdhci0) != 0) {
		pr_warn("SMHC: init failed, back to FEL\n");
	}
	disk_set_device(0, &card0);

	if (load_sdcard(&image, &e907) != 0) {
		pr_warn("SMHC: loading failed, back to FEL\n");
		goto _fel;
	}

	if (sunxi_remoteproc_reset(&e907) != DRIVER_OK || sunxi_remoteproc_prepare(&e907) != DRIVER_OK || sunxi_remoteproc_load(&e907) != DRIVER_OK) {
		pr_err("RISC-V E907: prepare or load failed\n");
		goto _fel;
	}
	pr_info("RISC-V ELF run addr: 0x%08x\n", (uint32_t)e907.entry);

	if (sunxi_remoteproc_start(&e907) != DRIVER_OK) {
		pr_err("RISC-V E907: start failed\n");
		goto _fel;
	}
	sunxi_remoteproc_dump(&e907);

	pr_info("RISC-V E907 Core now Running... \n");

	if (zImage_loader((unsigned char *)image.dest, &entry_point)) {
		pr_err("boot setup failed\n");
		goto _fel;
	}

	pr_info("booting linux...\n");

	arm32_mmu_disable();
	pr_info("disable mmu ok...\n");
	arm32_dcache_disable();
	pr_info("disable dcache ok...\n");
	arm32_icache_disable();
	pr_info("disable icache ok...\n");
	arm32_interrupt_disable();
	pr_info("free interrupt ok...\n");
	enable_kernel_smp();
	pr_info("enable kernel smp ok...\n");

	pr_info("jump to kernel address: 0x%x\n", image.dest);

	kernel_entry = (void (*)(int, int, unsigned int))entry_point;
	kernel_entry(0, ~0, (unsigned int)image.of_dest);

	// if kernel boot not success, jump to fel.
_fel:
	jmp_to_fel();

	return 0;
}
