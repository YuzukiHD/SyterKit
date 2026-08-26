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

#include <drivers/dram/dram.h>
#include <dt-compatible/dram-dt.h>
#include <drivers/i2c/i2c.h>
#include <drivers/remoteproc/remoteproc.h>
#include <dt-compatible/rtc-dt.h>
#include <drivers/mmc/sdcard.h>
#include <drivers/sid/sid.h>
#include <drivers/spi/spi.h>

#include <drivers/pmu/axp.h>
#include <dt-bindings/soc/sun55iw3.h>
#include <dt-compatible/i2c-dt.h>
#include <dt-compatible/mmc-dt.h>
#include <dt-compatible/remoteproc-dt.h>

#include <fdt_wrapper.h>
#include <lib/fatfs/ff.h>
#include <lib/fatfs/diskio.h>
#include <drivers/mmc/sdhci.h>
#include <uart.h>

static sunxi_dram_t dram;

#define CONFIG_BL31_FILENAME "bl31.bin"
#define CONFIG_BL31_LOAD_ADDR (0x48000000)

#define CONFIG_UBOOT_FILENAME "u-boot.bin"
#define CONFIG_UBOOT_LOAD_ADDR (0x4a000000)

#define CONFIG_SCP_FILENAME "scp.bin"
#define CONFIG_SCP_LOAD_ADDR (0x48100000)

#define CONFIG_SDMMC_SPEED_TEST_SIZE 1024 // (unit: 512B sectors)

#define CONFIG_DEFAULT_BOOTDELAY 3

#define CONFIG_HEAP_BASE (0x40800000)
#define CONFIG_HEAP_SIZE (16 * 1024 * 1024)

extern sunxi_serial_t uart_dbg;

extern void set_rpio_power_mode(void);
extern int sunxi_nsi_init(void);
extern void gicr_set_waker(void);

typedef struct atf_head {
	uint32_t jump_instruction; /* jumping to real code */
	uint8_t magic[8]; /* magic */
	uint32_t scp_base; /* scp openrisc core bin */
	uint32_t next_boot_base; /* next boot base for uboot */
	uint32_t nos_base; /* ARM SVC RUNOS base */
	uint32_t secureos_base; /* optee base */
	uint8_t version[8]; /* atf version */
	uint8_t platform[8]; /* platform information */
	uint32_t reserved[1]; /* stamp space, 16bytes align */
	uint32_t dram_para[32]; /* the dram param */
	uint64_t dtb_base; /* the address of dtb */
} atf_head_t;

#define FILENAME_MAX_LEN 16
typedef struct {
	uint8_t *bl31_dest;
	char bl31_filename[FILENAME_MAX_LEN];

	uint8_t *kernel_dest;
	char kernel_filename[FILENAME_MAX_LEN];

	uint8_t *scp_dest;
	char scp_filename[FILENAME_MAX_LEN];
} image_info_t;

image_info_t image;
static sdmmc_pdata_t boot_card;
static sunxi_rtc_t rtc;
static sunxi_sdhci_t boot_mmc;

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

static int load_sdcard(image_info_t *image, sdmmc_pdata_t *card)
{
	FATFS fs;
	FRESULT fret;
	int ret;
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

	printk_info("FATFS: read %s addr=%x\n", image->bl31_filename, (uint32_t)image->bl31_dest);
	ret = fatfs_loadimage(image->bl31_filename, image->bl31_dest);
	if (ret)
		return ret;

	printk_info("FATFS: read %s addr=%x\n", image->kernel_filename, (uint32_t)image->kernel_dest);
	ret = fatfs_loadimage(image->kernel_filename, image->kernel_dest);
	if (ret)
		return ret;

	printk_info("FATFS: read %s addr=%x\n", image->scp_filename, (uint32_t)image->scp_dest);
	ret = fatfs_loadimage(image->scp_filename, image->scp_dest);
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

void jmp_to_arm64(const sunxi_rtc_t *rtc, uint32_t addr)
{
	/* Set RTC data to current time_ms(), Save in RTC_FEL_INDEX */
	rtc_set_start_time_ms(rtc);

	/* set the cpu boot entry addr: */
	write32(RVBARADDR0_L, addr);
	write32(RVBARADDR0_H, 0);

	/* set cpu to AA64 execution state when the cpu boots into after a warm reset */
	asm volatile("mrc p15,0,r2,c12,c0,2");
	asm volatile("orr r2,r2,#(0x3<<0)");
	asm volatile("dsb");
	asm volatile("mcr p15,0,r2,c12,c0,2");
	asm volatile("isb");
_loop:
	asm volatile("wfi");
	goto _loop;
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

msh_declare_command(boot);
msh_define_help(boot, "boot to linux", "Usage: boot\n");
int cmd_boot(int argc, const char **argv)
{
	atf_head_t *atf_head = (atf_head_t *)image.bl31_dest;

	atf_head->dtb_base = CONFIG_UBOOT_LOAD_ADDR;
	atf_head->nos_base = CONFIG_UBOOT_LOAD_ADDR;

	atf_head->platform[0] = 0x00;
	atf_head->platform[1] = 0x52;
	atf_head->platform[2] = 0x41;
	atf_head->platform[3] = 0x57;
	atf_head->platform[4] = 0xbe;
	atf_head->platform[5] = 0xe9;
	atf_head->platform[6] = 0x00;
	atf_head->platform[7] = 0x00;

	printk_info("ATF: Kernel addr: 0x%08x\n", atf_head->nos_base);
	printk_info("ATF: Kernel DTB addr: 0x%08x\n", atf_head->dtb_base);

	clean_syterkit_data();

	gicr_set_waker();

	jmp_to_arm64(&rtc, CONFIG_BL31_LOAD_ADDR);

	printk_info("Back to SyterKit\n");

	// if kernel boot not success, jump to fel.
	jmp_to_fel();
	return 0;
}

msh_declare_command(reload);
msh_define_help(reload, "rescan TF Card and reload DTB", "Usage: reload\n");
int cmd_reload(int argc, const char **argv)
{
	if (sdmmc_init(&boot_card, &boot_mmc) != 0) {
		printk_error("SMHC: init failed\n");
		return 0;
	}
	disk_set_device(0, &boot_card);

	if (load_sdcard(&image, &boot_card) != 0) {
		printk_error("SMHC: loading failed\n");
		return 0;
	}
	return 0;
}

const msh_command_entry commands[] = {
	msh_define_command(boot),
	msh_define_command(reload),
	msh_command_end,
};

int main(void)
{
	axp_pmu_t axp2202;
	axp_pmu_t axp1530;
	sunxi_i2c_t i2c;
	sunxi_remoteproc_t e906;

	if (sunxi_serial_init_stdout() != 0)
		return -1;

	show_banner();
	if (sunxi_remoteproc_dt_read_alias(&e906, "e906", NULL) != DRIVER_OK) {
		printk_error("RISC-V E906: invalid devicetree configuration\n");
		return -1;
	}
	if (sunxi_rtc_dt_read_alias(&rtc, "rtc0") != DRIVER_OK || sunxi_i2c_dt_read_alias(&i2c, "i2c0") != DRIVER_OK || pmu_axp2202_config(&axp2202, &i2c) != DRIVER_OK ||
	    pmu_axp1530_config(&axp1530, &i2c) != DRIVER_OK || sunxi_sdhci_dt_read_alias(&boot_mmc, "mmc0") != DRIVER_OK) {
		printk_error("Board: invalid devicetree configuration\n");
		return -1;
	}

	sunxi_clk_init();

	set_rpio_power_mode();

	sunxi_clk_dump();

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

	pmu_axp2202_dump(&axp2202);
	pmu_axp1530_dump(&axp1530);

	if (sunxi_remoteproc_reset(&e906) != DRIVER_OK) {
		printk_error("RISC-V E906: reset failed\n");
		return -1;
	}

	/* Initialize the DRAM and enable memory management unit (MMU). */
	dram.pmu = &axp2202;
	dram.pmu_aux = &axp1530;
	if (sunxi_dram_dt_read_alias(&dram, "dram0") != DRIVER_OK) {
		printk_error("DRAM: invalid devicetree configuration\n");
		return -1;
	}
	uint32_t dram_size = sunxi_dram_init(&dram);

	sunxi_clk_dump();

	arm32_mmu_enable(dram.memory_base, dram_size);

	/* Initialize the small memory allocator. */
	malloc_init(CONFIG_HEAP_BASE, CONFIG_HEAP_SIZE);

	sunxi_nsi_init();

	/* Clear the image_info_t struct. */
	memset(&image, 0, sizeof(image_info_t));

	image.bl31_dest = (uint8_t *)CONFIG_BL31_LOAD_ADDR;
	image.kernel_dest = (uint8_t *)CONFIG_UBOOT_LOAD_ADDR;
	image.scp_dest = (uint8_t *)CONFIG_SCP_LOAD_ADDR;

	strcpy(image.bl31_filename, CONFIG_BL31_FILENAME);
	strcpy(image.kernel_filename, CONFIG_UBOOT_FILENAME);
	strcpy(image.scp_filename, CONFIG_SCP_FILENAME);

	/* Initialize the SD host controller. */
	if (sunxi_sdhci_init(&boot_mmc) != 0) {
		printk_error("SMHC: %s controller init failed\n", boot_mmc.name);
		goto _shell;
	} else {
		printk_info("SMHC: %s controller initialized\n", boot_mmc.name);
	}

	/* Initialize the SD card and check if initialization is successful. */
	if (sdmmc_init(&boot_card, &boot_mmc) != 0) {
		printk_warning("SMHC: init failed, Retrying...\n");
		mdelay(30);
		if (sdmmc_init(&boot_card, &boot_mmc) != 0) {
			printk_warning("SMHC: init failed\n");
			goto _shell;
		}
	}
	disk_set_device(0, &boot_card);

	/* Load the DTB, kernel image, and configuration data from the SD card. */
	if (load_sdcard(&image, &boot_card) != 0) {
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

	return 0;
}
