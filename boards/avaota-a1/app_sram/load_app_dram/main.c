/* SPDX-License-Identifier: GPL-2.0+ */

#include <common.h>
#include <cache.h>
#include <log.h>
#include <malloc.h>
#include <mmu.h>

#include <drivers/clk/clk.h>
#include <drivers/dram/dram.h>
#include <drivers/i2c/i2c.h>
#include <drivers/mmc/sdcard.h>
#include <drivers/mmc/sdhci.h>
#include <drivers/pmu/axp.h>
#include <drivers/remoteproc/remoteproc.h>
#include <drivers/serial/serial.h>
#include <dt-compatible/dram-dt.h>
#include <dt-compatible/i2c-dt.h>
#include <dt-compatible/mmc-dt.h>
#include <dt-compatible/remoteproc-dt.h>
#include <lib/fatfs/ff.h>
#include <lib/fatfs/diskio.h>

#define APP_DRAM_FILENAME "hello_world_dram.bin"
#define LOADER_HEAP_BASE 0x40800000U
#define LOADER_HEAP_SIZE 0x01000000U
#define APP_DRAM_LOAD_ADDR 0x42000000U
#define APP_DRAM_MAX_SIZE 0x01000000U
#define LOAD_CHUNK_SIZE 0x00020000U

extern void set_rpio_power_mode(void);

static int load_app_dram(FIL *file, uint32_t image_size)
{
	uint8_t *destination = (uint8_t *)(uintptr_t)APP_DRAM_LOAD_ADDR;
	uint32_t offset = 0;

	while (offset < image_size) {
		UINT bytes_read = 0;
		UINT bytes_to_read = image_size - offset;
		FRESULT result;

		if (bytes_to_read > LOAD_CHUNK_SIZE)
			bytes_to_read = LOAD_CHUNK_SIZE;
		result = f_read(file, destination + offset, bytes_to_read,
				&bytes_read);
		if (result != FR_OK) {
			pr_err("Bootloader: read %s failed at 0x%x: %d\n",
			       APP_DRAM_FILENAME, offset, result);
			return -1;
		}
		if (bytes_read != bytes_to_read) {
			pr_err("Bootloader: short read at 0x%x: %u/%u bytes\n",
			       offset, bytes_read, bytes_to_read);
			return -1;
		}
		offset += bytes_read;
	}

	return 0;
}

int main(void)
{
	axp_pmu_t axp2202;
	axp_pmu_t axp1530;
	sunxi_i2c_t i2c;
	sunxi_remoteproc_t e906;
	sunxi_sdhci_t mmc;
	sdmmc_pdata_t card = { 0 };
	sunxi_dram_t dram = { 0 };
	FATFS filesystem;
	FIL file;
	FSIZE_t file_size;
	uint64_t dram_end;
	uint32_t dram_size_mb;
	FRESULT fat_error;

	if (sunxi_serial_init_stdout() != 0)
		return -1;
	show_banner();
	if (sunxi_remoteproc_dt_read_alias(&e906, "e906", NULL) != DRIVER_OK ||
	    sunxi_i2c_dt_read_alias(&i2c, "i2c0") != DRIVER_OK ||
	    pmu_axp2202_config(&axp2202, &i2c) != DRIVER_OK ||
	    pmu_axp1530_config(&axp1530, &i2c) != DRIVER_OK ||
	    sunxi_sdhci_dt_read_alias(&mmc, "mmc0") != DRIVER_OK) {
		pr_err("Bootloader: invalid devicetree configuration\n");
		return -1;
	}

	sunxi_clk_init();
	set_rpio_power_mode();
	sunxi_i2c_init(&i2c);
	pmu_axp2202_init(&axp2202);
	pmu_axp1530_init(&axp1530);
	pmu_axp2202_set_vol(&axp2202, "dcdc1", 1100, 1);
	pmu_axp1530_set_dual_phase(&axp1530);
	pmu_axp1530_set_vol(&axp1530, "dcdc1", 1100, 1);
	pmu_axp1530_set_vol(&axp1530, "dcdc2", 1100, 1);
	pmu_axp2202_set_vol(&axp2202, "dcdc2", 920, 1);
	pmu_axp2202_set_vol(&axp2202, "dcdc3", 1160, 1);
	pmu_axp2202_set_vol(&axp2202, "dcdc4", 3300, 1);
	if (sunxi_remoteproc_reset(&e906) != DRIVER_OK) {
		pr_err("Bootloader: E906 reset failed\n");
		return -1;
	}

	if (sunxi_dram_dt_read_alias(&dram, "dram0") != DRIVER_OK) {
		pr_err("Bootloader: invalid DRAM configuration\n");
		return -1;
	}
	dram_size_mb = sunxi_dram_init(&dram);
	if (dram_size_mb == 0U) {
		pr_err("Bootloader: DRAM initialization failed\n");
		return -1;
	}
	pr_info("Bootloader: DRAM ready, %u MiB at 0x%lx\n", dram_size_mb,
		(unsigned long)dram.memory_base);
	arm32_mmu_enable(dram.memory_base, dram_size_mb);
	if (malloc_init(LOADER_HEAP_BASE, LOADER_HEAP_SIZE) != 0) {
		pr_err("Bootloader: heap initialization failed\n");
		return -1;
	}

	if (sunxi_sdhci_init(&mmc) != 0 || sdmmc_init(&card, &mmc) != 0) {
		pr_err("Bootloader: SD card initialization failed\n");
		return -1;
	}
	disk_set_device(0, &card);
	fat_error = f_mount(&filesystem, "", 1);
	if (fat_error != FR_OK) {
		pr_err("Bootloader: FATFS mount failed: %d\n", fat_error);
		return -1;
	}
	fat_error = f_open(&file, APP_DRAM_FILENAME, FA_OPEN_EXISTING | FA_READ);
	if (fat_error != FR_OK) {
		pr_err("Bootloader: open %s failed: %d\n", APP_DRAM_FILENAME,
		       fat_error);
		f_mount(NULL, "", 0);
		return -1;
	}

	file_size = f_size(&file);
	dram_end = (uint64_t)dram.memory_base +
		   (uint64_t)dram_size_mb * 1024U * 1024U;
	if (file_size == 0 || file_size > APP_DRAM_MAX_SIZE) {
		pr_err("Bootloader: invalid DRAM app size: %llu bytes\n",
		       (unsigned long long)file_size);
		f_close(&file);
		f_mount(NULL, "", 0);
		return -1;
	}
	if (APP_DRAM_LOAD_ADDR < dram.memory_base ||
	    (uint64_t)APP_DRAM_LOAD_ADDR + APP_DRAM_MAX_SIZE > dram_end) {
		pr_err("Bootloader: DRAM app window 0x%x-0x%llx is outside DRAM\n",
		       APP_DRAM_LOAD_ADDR,
		       (unsigned long long)((uint64_t)APP_DRAM_LOAD_ADDR +
					    APP_DRAM_MAX_SIZE));
		f_close(&file);
		f_mount(NULL, "", 0);
		return -1;
	}
	pr_info("Bootloader: loading %s (%llu bytes) to 0x%x\n",
		APP_DRAM_FILENAME, (unsigned long long)file_size,
		APP_DRAM_LOAD_ADDR);
	if (load_app_dram(&file, (uint32_t)file_size) != 0) {
		f_close(&file);
		f_mount(NULL, "", 0);
		return -1;
	}
	f_close(&file);
	f_mount(NULL, "", 0);

	pr_info("Bootloader: entering DRAM app at 0x%x\n", APP_DRAM_LOAD_ADDR);
	flush_dcache_range(APP_DRAM_LOAD_ADDR,
			   (uint64_t)APP_DRAM_LOAD_ADDR + file_size);
	arm32_icache_invalidate_all();
	clean_syterkit_data();
	((void (*)(void))(uintptr_t)APP_DRAM_LOAD_ADDR)();

	pr_err("Bootloader: DRAM app returned unexpectedly\n");
	for (;;)
		__asm__ volatile("wfi");
}
