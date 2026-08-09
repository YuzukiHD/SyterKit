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
#include <malloc.h>
#include <stdlib.h>
#include <string.h>

#include <cli/cli.h>
#include <cli/cli_shell.h>
#include <cli/cli_termesc.h>

#include <drivers/clk.h>
#include <drivers/dram.h>
#include <dt-compatible/dram-dt.h>
#include <drivers/i2c.h>
#include <drivers/remoteproc.h>
#include <drivers/rtc.h>
#include <drivers/mmc/sdcard.h>
#include <drivers/sid.h>
#include <drivers/spi.h>

#include <drivers/pmu/axp.h>
#include <dt-compatible/i2c-dt.h>
#include <dt-compatible/mmc-dt.h>
#include <dt-compatible/pmu-dt.h>
#include <dt-compatible/remoteproc-dt.h>

#include <fdt_wrapper.h>
#include <lib/fatfs/ff.h>
#include <lib/fatfs/diskio.h>
#include <drivers/mmc/sdhci.h>
#include <uart.h>

#define CONFIG_SDMMC_SPEED_TEST_SIZE 1024// (unit: 512B sectors)

#define CONFIG_DEFAULT_BOOTDELAY 3

#define CONFIG_HEAP_BASE (0x40800000)
#define CONFIG_HEAP_SIZE (16 * 1024 * 1024)

extern sunxi_serial_t uart_dbg;



extern void set_rpio_power_mode(void);
extern int sunxi_nsi_init(void);

typedef struct atf_head {
	uint32_t jump_instruction; /* jumping to real code */
	uint8_t magic[8];		   /* magic */
	uint32_t scp_base;		   /* scp openrisc core bin */
	uint32_t next_boot_base;   /* next boot base for uboot */
	uint32_t nos_base;		   /* ARM SVC RUNOS base */
	uint32_t secureos_base;	   /* optee base */
	uint8_t version[8];		   /* atf version */
	uint8_t platform[8];	   /* platform information */
	uint32_t reserved[1];	   /* stamp space, 16bytes align */
	uint32_t dram_para[32];	   /* the dram param */
	uint64_t dtb_base;		   /* the address of dtb */
} atf_head_t;

#define CHUNK_SIZE 0x20000

static int fatfs_loadimage(const char *filename, BYTE *dest) {
	FIL file;
	UINT byte_to_read = CHUNK_SIZE;
	UINT byte_read;
	UINT total_read = 0;
	FRESULT fret;
	int ret = 1;
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

	printk_info("FATFS: read in %ums at %.2fMB/S\n", time, (f32) (total_read / time) / 1024.0f);

open_fail:
	return ret;
}

static int load_sdcard(sunxi_remoteproc_t *remoteproc,
		       sdmmc_pdata_t *card) {
	FATFS fs;
	FRESULT fret;
	int ret;
	size_t index;
	uint32_t start;

	uint32_t test_time;
	start = time_ms();
	sdmmc_blk_read(card, (uint8_t *) (SDRAM_BASE), 0,
		       CONFIG_SDMMC_SPEED_TEST_SIZE);
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
		const sunxi_remoteproc_firmware_t *firmware =
				&remoteproc->firmware[index];

		printk_info("FATFS: read %s addr=%x\n", firmware->name,
			    (uint32_t) firmware->load_address);
		ret = fatfs_loadimage(firmware->name,
				      (BYTE *) firmware->load_address);
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

int main(void) {
	sunxi_ccu_t ccu;
	sunxi_dram_t dram;
	axp_pmu_t primary_pmu;
	axp_pmu_t secondary_pmu;
	sdmmc_pdata_t card = {0};
	sunxi_i2c_t i2c;
	sunxi_remoteproc_t e906;
	sunxi_sdhci_t sdmmc;

	show_banner();
	if (sunxi_i2c_dt_read_alias(&i2c, "i2c0") != DRIVER_OK ||
	    sunxi_pmu_dt_read_alias(&primary_pmu, "pmu0", &i2c) != DRIVER_OK ||
	    sunxi_pmu_dt_read_alias(&secondary_pmu, "pmu1", &i2c) != DRIVER_OK ||
	    sunxi_sdhci_dt_read_alias(&sdmmc, "mmc0") != DRIVER_OK ||
	    sunxi_remoteproc_dt_read_alias(&e906, "e906", NULL) != DRIVER_OK) {
		printk_error("Board: invalid devicetree configuration\n");
		return -1;
	}

	if (sunxi_ccu_dt_read(&ccu) != DRIVER_OK) {
		printk_error("CCU: invalid devicetree configuration\n");
		return -1;
	}

	sunxi_clk_init(&ccu);

	set_rpio_power_mode();

	sunxi_clk_dump(&ccu);

	sunxi_i2c_init(&i2c);

	pmu_axp2202_init(&primary_pmu);

	pmu_axp1530_init(&secondary_pmu);

	pmu_axp2202_set_vol(&primary_pmu, "dcdc1", 1100, 1);

	pmu_axp1530_set_dual_phase(&secondary_pmu);
	pmu_axp1530_set_vol(&secondary_pmu, "dcdc1", 1100, 1);
	pmu_axp1530_set_vol(&secondary_pmu, "dcdc2", 1100, 1);

	pmu_axp2202_set_vol(&primary_pmu, "dcdc2", 920, 1);
	pmu_axp2202_set_vol(&primary_pmu, "dcdc3", 1160, 1);
	pmu_axp2202_set_vol(&primary_pmu, "dcdc4", 3300, 1);

	pmu_axp2202_dump(&primary_pmu);
	pmu_axp1530_dump(&secondary_pmu);

	if (sunxi_remoteproc_reset(&e906) != DRIVER_OK) {
		printk_error("RISC-V E906: reset failed\n");
		return -1;
	}

	/* Initialize the DRAM and enable memory management unit (MMU). */
	if (sunxi_dram_dt_read_alias(&dram, "dram0", NULL, NULL) != DRIVER_OK) {
		printk_error("DRAM: invalid devicetree configuration\n");
		return -1;
	}
	uint32_t dram_size = sunxi_dram_init(&dram);

	sunxi_clk_dump(&ccu);

	arm32_mmu_enable(SDRAM_BASE, dram_size);

	/* Initialize the small memory allocator. */
	malloc_init(CONFIG_HEAP_BASE, CONFIG_HEAP_SIZE);

	sunxi_nsi_init();

	/* Initialize the SD host controller. */
	if (sunxi_sdhci_init(&sdmmc) != 0) {
		printk_error("SMHC: %s controller init failed\n", sdmmc.name);
		goto _shell;
	} else {
		printk_info("SMHC: %s controller initialized\n", sdmmc.name);
	}

	/* Initialize the SD card and check if initialization is successful. */
	if (sdmmc_init(&card, &sdmmc) != 0) {
		printk_warning("SMHC: init failed, Retrying...\n");
		mdelay(30);
		if (sdmmc_init(&card, &sdmmc) != 0) {
			printk_warning("SMHC: init failed\n");
			goto _shell;
		}
	}
	disk_set_device(0, &card);

	/* Load the DTB, kernel image, and configuration data from the SD card. */
	if (load_sdcard(&e906, &card) != 0) {
		printk_warning("SMHC: loading failed\n");
		goto _shell;
	}

	if (sunxi_remoteproc_prepare(&e906) != DRIVER_OK ||
	    sunxi_remoteproc_load(&e906) != DRIVER_OK) {
		printk_error("RISC-V E906: prepare or load failed\n");
		goto _shell;
	}
	printk_info("RISC-V ELF run addr: 0x%08x\n", (uint32_t) e906.entry);

	if (sunxi_remoteproc_start(&e906) != DRIVER_OK) {
		printk_error("RISC-V E906: start failed\n");
		goto _shell;
	}
	sunxi_remoteproc_dump(&e906);

	printk_info("RISC-V E906 Core now Running... \n");

	abort();

_shell:
	syterkit_shell_attach(NULL);

	return 0;
}
