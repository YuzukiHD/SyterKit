/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <config.h>
#include <log.h>

#include <mmu.h>
#include <common.h>
#include <jmp.h>

#include <image_loader.h>

#include "sys-dram.h"
#include "sys-sdcard.h"
#include "sys-sid.h"
#include "sys-spi.h"

#include "libfdt.h"
#include "ff.h"

#define CONFIG_KERNEL_FILENAME "zImage"
#define CONFIG_DTB_FILENAME "sunxi.dtb"
#define CONFIG_RISCV_ELF_FILENAME "e907.elf"

#define CONFIG_SDMMC_SPEED_TEST_SIZE 1024// (unit: 512B sectors)

#define CONFIG_RISCV_ELF_LOADADDR (0x40008000)
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

	unsigned int elf_offset;
	unsigned char *elf_dest;

	char filename[FILENAME_MAX_LEN];
	char of_filename[FILENAME_MAX_LEN];
	char elf_filename[FILENAME_MAX_LEN];
} image_info_t;

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

extern sunxi_spi_t sunxi_spi0;

extern sdhci_t sdhci0;

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

	printk_info("FATFS: read %s addr=%x\n", image->of_filename, (unsigned int) image->of_dest);
	ret = fatfs_loadimage(image->of_filename, image->of_dest);
	if (ret)
		return ret;

	printk_info("FATFS: read %s addr=%x\n", image->filename, (unsigned int) image->dest);
	ret = fatfs_loadimage(image->filename, image->dest);
	if (ret)
		return ret;

	printk_info("FATFS: read %s addr=%x\n", image->elf_filename, (unsigned int) image->elf_dest);
	ret = fatfs_loadimage(image->elf_filename, image->elf_dest);
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
	sunxi_serial_init(&uart_dbg);

	sunxi_serial_init(&uart_e907);

	show_banner();

	sunxi_clk_init();

	sunxi_dram_init(&dram_para);

	uint32_t entry_point = 0;
	void (*kernel_entry)(int zero, int arch, unsigned int params);

	sunxi_clk_dump();

	memset(&image, 0, sizeof(image_info_t));

	image.of_dest = (uint8_t *) CONFIG_DTB_LOAD_ADDR;
	image.dest = (uint8_t *) CONFIG_KERNEL_LOAD_ADDR;
	image.elf_dest = (uint8_t *) CONFIG_RISCV_ELF_LOADADDR;

	strcpy(image.filename, CONFIG_KERNEL_FILENAME);
	strcpy(image.of_filename, CONFIG_DTB_FILENAME);
	strcpy(image.elf_filename, CONFIG_RISCV_ELF_FILENAME);

	if (sunxi_sdhci_init(&sdhci0) != 0) {
		printk_error("SMHC: %s controller init failed\n", sdhci0.name);
	} else {
		printk_info("SMHC: %s controller v%x initialized\n", sdhci0.name, sdhci0.reg->vers);
	}
	if (sdmmc_init(&card0, &sdhci0) != 0) {
		printk_warning("SMHC: init failed, back to FEL\n");
	}

	if (load_sdcard(&image) != 0) {
		printk_warning("SMHC: loading failed, back to FEL\n");
		goto _fel;
	}

	sunxi_e907_clock_reset();

	uint32_t elf_run_addr = elf32_get_entry_addr((phys_addr_t) image.dest);
	printk_info("RISC-V ELF run addr: 0x%08x\n", elf_run_addr);

	if (load_elf32_image((phys_addr_t) image.dest)) {
		printk_error("RISC-V ELF load FAIL\n");
	}

	sunxi_e907_clock_init(elf_run_addr);

	dump_e907_clock();

	printk_info("RISC-V E907 Core now Running... \n");

	if (zImage_loader((unsigned char *) image.dest, &entry_point)) {
		printk_error("boot setup failed\n");
		goto _fel;
	}

	printk_info("booting linux...\n");

	arm32_mmu_disable();
	printk_info("disable mmu ok...\n");
	arm32_dcache_disable();
	printk_info("disable dcache ok...\n");
	arm32_icache_disable();
	printk_info("disable icache ok...\n");
	arm32_interrupt_disable();
	printk_info("free interrupt ok...\n");
	enable_kernel_smp();
	printk_info("enable kernel smp ok...\n");

	printk_info("jump to kernel address: 0x%x\n", image.dest);

	kernel_entry = (void (*)(int, int, unsigned int)) entry_point;
	kernel_entry(0, ~0, (unsigned int) image.of_dest);

	// if kernel boot not success, jump to fel.
_fel:
	jmp_to_fel();

	return 0;
}
