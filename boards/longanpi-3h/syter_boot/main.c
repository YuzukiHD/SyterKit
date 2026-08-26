/* SPDX-License-Identifier: GPL-2.0+ */

#include <drivers/serial/serial.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <config.h>
#include <log.h>
#include <drivers/clk/clk.h>
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

#include <image/image_loader.h>

#include <drivers/dram/dram.h>
#include <dt-compatible/dram-dt.h>
#include <drivers/i2c/i2c.h>
#include <drivers/pmu/axp.h>
#include <dt-compatible/i2c-dt.h>
#include <dt-compatible/mmc-dt.h>
#include <dt-compatible/rtc-dt.h>
#include <drivers/mmc/sdcard.h>
#include <drivers/sid/sid.h>
#include <drivers/spi/spi.h>

#include "fdt_wrapper.h"
#include <lib/fatfs/ff.h>
#include <lib/fatfs/diskio.h>
#include <lib/fdt/libfdt.h>
#include "uart.h"

static sunxi_dram_t dram;

#define CONFIG_KERNEL_FILENAME "zImage"
#define CONFIG_DTB_FILENAME "sunxi.dtb"
#define CONFIG_CONFIG_FILENAME "config.txt"

#define CONFIG_SDMMC_SPEED_TEST_SIZE 1024 // (unit: 512B sectors)

#define CONFIG_DTB_LOAD_ADDR (0x41008000)
#define CONFIG_KERNEL_LOAD_ADDR (0x41800000)
#define CONFIG_CONFIG_LOAD_ADDR (0x40008000)
#define CONFIG_HEAP_BASE (0x40800000)
#define CONFIG_HEAP_SIZE (16 * 1024 * 1024)

#define CONFIG_DEFAULT_BOOTDELAY 5

#define FILENAME_MAX_LEN 64
typedef struct {
	uint8_t *dest;
	uint8_t *of_dest;
	char filename[FILENAME_MAX_LEN];
	char of_filename[FILENAME_MAX_LEN];
} image_info_t;

extern sunxi_serial_t uart_dbg;

static sunxi_sdhci_t boot_mmc;
static sdmmc_pdata_t boot_card;

image_info_t image;

#define CHUNK_SIZE 0x20000

static int fatfs_loadimage(char *filename, BYTE *dest)
{
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

	printk_info("FATFS: read in %ums at %.2fMB/S\n", time, (f32)(total_read / time) / 1024.0f);

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
	sdmmc_blk_read(&boot_card, (uint8_t *)(dram.memory_base), 0, CONFIG_SDMMC_SPEED_TEST_SIZE);
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

	/* load DTB */
	printk_info("FATFS: read %s addr=%x\n", image->of_filename, (uint32_t)image->of_dest);
	ret = fatfs_loadimage(image->of_filename, image->of_dest);
	if (ret)
		return ret;

	/* load Kernel */
	printk_info("FATFS: read %s addr=%x\n", image->filename, (uint32_t)image->dest);
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
	printk_info("FATFS: done in %ums\n", time_ms() - start);

	return 0;
}

static int abortboot_single_key(int bootdelay)
{
	int abort = 0;
	unsigned long ts;

	printk_info("Hit any key to stop autoboot: %2d ", bootdelay);

	/* Check if key already pressed */
	if (tstc()) { /* we got a key press */
		uart_getchar(); /* consume input */
		printk(LOG_LEVEL_MUTE, "\b\b\b%2d", bootdelay);
		abort = 1; /* don't auto boot */
	}

	while ((bootdelay > 0) && (!abort)) {
		--bootdelay;
		/* delay 1000 ms */
		ts = time_ms();
		do {
			if (tstc()) { /* we got a key press */
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

static void set_pmu_fin_voltage(axp_pmu_t *pmu, char *power_name, uint32_t voltage)
{
	int temp_vol, src_vol = pmu_axp1530_get_vol(pmu, power_name);
	if (src_vol > voltage) {
		for (temp_vol = src_vol; temp_vol >= voltage; temp_vol -= 50) {
			pmu_axp1530_set_vol(pmu, power_name, temp_vol, 1);
		}
	} else if (src_vol < voltage) {
		for (temp_vol = src_vol; temp_vol <= voltage; temp_vol += 50) {
			pmu_axp1530_set_vol(pmu, power_name, temp_vol, 1);
		}
	}
	mdelay(30); /* Delay 300ms for pmu bootup */
}

msh_declare_command(reload);
msh_define_help(reload, "rescan TF Card and reload DTB, Kernel zImage", "Usage: reload\n");
int cmd_reload(int argc, const char **argv)
{
	if (sdmmc_init(&boot_card, &boot_mmc) != 0) {
		printk_error("SMHC: init failed\n");
		return 0;
	}
	disk_set_device(0, &boot_card);

	if (load_sdcard(&image) != 0) {
		printk_error("SMHC: loading failed\n");
		return 0;
	}
	return 0;
}

msh_declare_command(boot);
msh_define_help(boot, "boot to linux", "Usage: boot\n");
int cmd_boot(int argc, const char **argv)
{
	/* Initialize variables for kernel entry point and SD card access. */
	uint32_t entry_point = 0;
	void (*kernel_entry)(int zero, int arch, uint32_t params);

	/* Set up boot parameters for the kernel. */
	if (zImage_loader((uint8_t *)image.dest, &entry_point)) {
		printk_error("boot setup failed\n");
		abort();
	}

	/* Disable MMU, data cache, instruction cache, interrupts */
	clean_syterkit_data();
	/* Debug message to indicate the kernel address that the system is jumping to. */
	printk_info("jump to kernel address: 0x%x\n\n", image.dest);

	/* Jump to the kernel entry point. */
	kernel_entry = (void (*)(int, int, uint32_t))entry_point;
	kernel_entry(0, ~0, (uint32_t)image.of_dest);
	return 0;
}

const msh_command_entry commands[] = {
	msh_define_command(reload),
	msh_define_command(boot),
	msh_command_end,
};

/* 
 * main function for the bootloader. Initializes and sets up the system, loads the kernel and device tree binary from
 * an SD card, sets boot arguments, and boots the kernel. If the kernel fails to boot, the function jumps to FEL mode.
 */
int main(void)
{
	axp_pmu_t pmu;
	sunxi_i2c_t i2c;
	sunxi_rtc_t rtc;
	/* Initialize the debug serial interface. */

	/* Display the bootloader banner. */
	if (sunxi_serial_init_stdout() != 0)
		return -1;
	show_banner();
	if (sunxi_i2c_dt_read_alias(&i2c, "i2c0") != DRIVER_OK || pmu_axp1530_config(&pmu, &i2c) != DRIVER_OK) {
		printk_error("PMU: invalid devicetree configuration\n");
		return -1;
	}
	if (sunxi_sdhci_dt_read_alias(&boot_mmc, "mmc0") != DRIVER_OK) {
		printk_error("SMHC: invalid devicetree configuration\n");
		return -1;
	}
	if (sunxi_rtc_dt_read_alias(&rtc, "rtc0") != DRIVER_OK) {
		printk_error("RTC: invalid devicetree configuration\n");
		return -1;
	}

	/* Initialize the system clock. */

	sunxi_clk_init();

	/* Check rtc fel flag. if set flag, goto fel */
	if (rtc_probe_fel_flag(&rtc)) {
		printk_info("RTC: get fel flag, jump to fel mode.\n");
		clean_syterkit_data();
		rtc_clear_fel_flag(&rtc);
		sunxi_clk_reset();
		mdelay(100);
		goto _fel;
	}

	sunxi_i2c_init(&i2c);

	pmu_axp1530_init(&pmu);
	set_pmu_fin_voltage(&pmu, "dcdc2", 1100);
	set_pmu_fin_voltage(&pmu, "dcdc3", 1100);

	/* Initialize the DRAM and enable memory management unit (MMU). */
	if (sunxi_dram_dt_read_alias(&dram, "dram0") != DRIVER_OK) {
		printk_error("DRAM: invalid devicetree configuration\n");
		return -1;
	}
	uint32_t dram_size = sunxi_dram_init(&dram);
	arm32_mmu_enable(dram.memory_base, dram_size);

	/* Initialize the small memory allocator. */
	malloc_init(CONFIG_HEAP_BASE, CONFIG_HEAP_SIZE);

	/* Clear the image_info_t struct. */
	memset(&image, 0, sizeof(image_info_t));

	/* Set the destination address for the device tree binary (DTB), kernel image, and configuration data. */
	image.of_dest = (uint8_t *)CONFIG_DTB_LOAD_ADDR;
	image.dest = (uint8_t *)CONFIG_KERNEL_LOAD_ADDR;

	/* Copy the filenames for the DTB, kernel image, and configuration data. */
	strcpy(image.filename, CONFIG_KERNEL_FILENAME);
	strcpy(image.of_filename, CONFIG_DTB_FILENAME);

	/* Initialize the SD host controller. */
	if (sunxi_sdhci_init(&boot_mmc) != 0) {
		printk_error("SMHC: %s controller init failed\n", boot_mmc.name);
		goto _shell;
	} else {
		printk_info("SMHC: %s controller initialized\n", boot_mmc.name);
	}

	/* Initialize the SD card and check if initialization is successful. */
	if (sdmmc_init(&boot_card, &boot_mmc) != 0) {
		printk_warning("SMHC: init failed\n");
		goto _shell;
	}
	disk_set_device(0, &boot_card);

	/* Load the DTB, kernel image, and configuration data from the SD card. */
	if (load_sdcard(&image) != 0) {
		printk_warning("SMHC: loading failed\n");
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
