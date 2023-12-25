/* SPDX-License-Identifier: Apache-2.0 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <config.h>
#include <log.h>

#include <mmu.h>
#include <common.h>
#include <jmp.h>

#include "sys-dram.h"
#include "sys-sdcard.h"
#include "sys-sid.h"
#include "sys-spi.h"
#include "sys-dma.h"
#include "sys-spi-nand.h"

#include "libfdt.h"
#include "ff.h"

#define CONFIG_KERNEL_FILENAME "zImage"
#define CONFIG_DTB_FILENAME "sunxi.dtb"

#define CONFIG_SDMMC_SPEED_TEST_SIZE 1024// (unit: 512B sectors)

#define CONFIG_DTB_LOAD_ADDR (0x41008000)
#define CONFIG_KERNEL_LOAD_ADDR (0x41800000)

// 128KB erase sectors, so place them starting from 2nd sector
#define CONFIG_SPINAND_DTB_ADDR (128 * 2048)
#define CONFIG_SPINAND_KERNEL_ADDR (256 * 2048)

#define FILENAME_MAX_LEN 64
typedef struct {
    unsigned int offset;
    unsigned int length;
    unsigned char *dest;

    unsigned int of_offset;
    unsigned char *of_dest;

    char filename[FILENAME_MAX_LEN];
    char of_filename[FILENAME_MAX_LEN];
} image_info_t;

/* Linux zImage Header */
#define LINUX_ZIMAGE_MAGIC 0x016f2818
typedef struct {
    unsigned int code[9];
    unsigned int magic;
    unsigned int start;
    unsigned int end;
} linux_zimage_header_t;

extern sunxi_serial_t uart_dbg;

extern dram_para_t dram_para;

extern sunxi_spi_t sunxi_spi0;

extern sdhci_t sdhci0;

image_info_t image;

unsigned int code[9];
unsigned int magic;
unsigned int start;
unsigned int end;

static int boot_image_setup(unsigned char *addr, unsigned int *entry) {
    linux_zimage_header_t *zimage_header = (linux_zimage_header_t *) addr;

    printk(LOG_LEVEL_INFO, "Linux zImage->code  = 0x");
    for (int i = 0; i < 9; i++)
        printk(LOG_LEVEL_MUTE, "%x", code[i]);

    printk(LOG_LEVEL_MUTE, "\n");
    printk(LOG_LEVEL_DEBUG, "Linux zImage->magic = 0x%x\n",
           zimage_header->magic);
    printk(LOG_LEVEL_DEBUG, "Linux zImage->start = 0x%x\n",
           (unsigned int) addr + zimage_header->start);
    printk(LOG_LEVEL_DEBUG, "Linux zImage->end   = 0x%x\n",
           (unsigned int) addr + zimage_header->end);

    if (zimage_header->magic == LINUX_ZIMAGE_MAGIC) {
        *entry = ((unsigned int) addr + zimage_header->start);
        return 0;
    }

    printk(LOG_LEVEL_ERROR, "unsupGPIO_PORTed kernel image\n");

    return -1;
}

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
        printk(LOG_LEVEL_ERROR,
               "FATFS: open, filename: [%s]: error %d\n", filename,
               fret);
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
        printk(LOG_LEVEL_ERROR, "FATFS: read: error %d\n", fret);
        ret = -1;
        goto read_fail;
    }
    ret = 0;

read_fail:
    fret = f_close(&file);

    printk(LOG_LEVEL_DEBUG, "FATFS: read in %ums at %.2fMB/S\n", time,
           (f32) (total_read / time) / 1024.0f);

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
    sdmmc_blk_read(&card0, (uint8_t *) (SDRAM_BASE), 0,
                   CONFIG_SDMMC_SPEED_TEST_SIZE);
    test_time = time_ms() - start;
    printk(LOG_LEVEL_DEBUG, "SDMMC: speedtest %uKB in %ums at %uKB/S\n",
           (CONFIG_SDMMC_SPEED_TEST_SIZE * 512) / 1024, test_time,
           (CONFIG_SDMMC_SPEED_TEST_SIZE * 512) / test_time);

    start = time_ms();

    fret = f_mount(&fs, "", 1);
    if (fret != FR_OK) {
        printk(LOG_LEVEL_ERROR, "FATFS: mount error: %d\n", fret);
        return -1;
    } else {
        printk(LOG_LEVEL_DEBUG, "FATFS: mount OK\n");
    }

    printk(LOG_LEVEL_INFO, "FATFS: read %s addr=%x\n", image->of_filename,
           (unsigned int) image->of_dest);
    ret = fatfs_loadimage(image->of_filename, image->of_dest);
    if (ret)
        return ret;

    printk(LOG_LEVEL_INFO, "FATFS: read %s addr=%x\n", image->filename,
           (unsigned int) image->dest);
    ret = fatfs_loadimage(image->filename, image->dest);
    if (ret)
        return ret;

    /* umount fs */
    fret = f_mount(0, "", 0);
    if (fret != FR_OK) {
        printk(LOG_LEVEL_ERROR, "FATFS: unmount error %d\n", fret);
        return -1;
    } else {
        printk(LOG_LEVEL_DEBUG, "FATFS: unmount OK\n");
    }
    printk(LOG_LEVEL_DEBUG, "FATFS: done in %ums\n", time_ms() - start);

    return 0;
}

int load_spi_nand(sunxi_spi_t *spi, image_info_t *image) {
    linux_zimage_header_t *hdr;
    unsigned int size;
    uint64_t start, time;

    if (spi_nand_detect(spi) != 0)
        return -1;

    /* get dtb size and read */
    spi_nand_read(spi, image->of_dest, CONFIG_SPINAND_DTB_ADDR, (uint32_t) sizeof(struct fdt_header));
    if (fdt_check_header(image->of_dest)) {
        printk(LOG_LEVEL_ERROR, "SPI-NAND: DTB verification failed\n");
        return -1;
    }

    size = fdt_totalsize(image->of_dest);
    printk(LOG_LEVEL_DEBUG,
           "SPI-NAND: dt blob: Copy from 0x%08x to 0x%08lx size:0x%08x\n",
           CONFIG_SPINAND_DTB_ADDR, (uint32_t) image->of_dest, size);
    start = time_us();
    spi_nand_read(spi, image->of_dest, CONFIG_SPINAND_DTB_ADDR,
                  (uint32_t) size);
    time = time_us() - start;
    printk(LOG_LEVEL_INFO,
           "SPI-NAND: read dt blob of size %u at %.2fMB/S\n", size,
           (f32) (size / time));

    /* get kernel size and read */
    spi_nand_read(spi, image->dest, CONFIG_SPINAND_KERNEL_ADDR,
                  (uint32_t) sizeof(linux_zimage_header_t));
    hdr = (linux_zimage_header_t *) image->dest;
    if (hdr->magic != LINUX_ZIMAGE_MAGIC) {
        printk(LOG_LEVEL_DEBUG,
               "SPI-NAND: zImage verification failed\n");
        return -1;
    }
    size = hdr->end - hdr->start;
    printk(LOG_LEVEL_DEBUG,
           "SPI-NAND: Image: Copy from 0x%08x to 0x%08lx size:0x%08x\n",
           CONFIG_SPINAND_KERNEL_ADDR, (uint32_t) image->dest, size);
    start = time_us();
    spi_nand_read(spi, image->dest, CONFIG_SPINAND_KERNEL_ADDR,
                  (uint32_t) size);
    time = time_us() - start;
    printk(LOG_LEVEL_INFO,
           "SPI-NAND: read Image of size %u at %.2fMB/S\n", size,
           (f32) (size / time));

    return 0;
}



int main(void) {
    sunxi_serial_init(&uart_dbg);

    show_banner();

    sunxi_clk_init();

    sunxi_dram_init(&dram_para);

    unsigned int entry_point = 0;
    void (*kernel_entry)(int zero, int arch, unsigned int params);

    sunxi_clk_dump();

    memset(&image, 0, sizeof(image_info_t));

    image.of_dest = (uint8_t *) CONFIG_DTB_LOAD_ADDR;
    image.dest = (uint8_t *) CONFIG_KERNEL_LOAD_ADDR;

    strcpy(image.filename, CONFIG_KERNEL_FILENAME);
    strcpy(image.of_filename, CONFIG_DTB_FILENAME);

    if (sunxi_sdhci_init(&sdhci0) != 0) {
        printk(LOG_LEVEL_ERROR, "SMHC: %s controller init failed\n",
               sdhci0.name);
    } else {
        printk(LOG_LEVEL_INFO,
               "SMHC: %s controller v%x initialized\n", sdhci0.name,
               sdhci0.reg->vers);
    }
    if (sdmmc_init(&card0, &sdhci0) != 0) {
        printk(LOG_LEVEL_WARNING, "SMHC: init failed, trying SPI\n");
        goto _spi;
    }

    if (load_sdcard(&image) != 0) {
        printk(LOG_LEVEL_WARNING,
               "SMHC: loading failed, trying SPI\n");
    } else {
        goto _boot;
    }

_spi:
    dma_init();
    dma_test((uint32_t *) CONFIG_DTB_LOAD_ADDR,
             (uint32_t *) CONFIG_KERNEL_LOAD_ADDR);
    printk(LOG_LEVEL_DEBUG, "SPI: init\n");
    if (sunxi_spi_init(&sunxi_spi0) != 0) {
        printk(LOG_LEVEL_ERROR, "SPI: init failed\n");
    }

    if (load_spi_nand(&sunxi_spi0, &image) != 0) {
        printk(LOG_LEVEL_ERROR, "SPI-NAND: loading failed\n");
    }

    sunxi_spi_disable(&sunxi_spi0);
    dma_exit();

_boot:
    if (boot_image_setup((unsigned char *) image.dest, &entry_point)) {
        printk(LOG_LEVEL_ERROR, "boot setup failed\n");
        abort();
    }

    printk(LOG_LEVEL_INFO, "booting linux...\n");

    arm32_mmu_disable();
    printk(LOG_LEVEL_INFO, "disable mmu ok...\n");
    arm32_dcache_disable();
    printk(LOG_LEVEL_INFO, "disable dcache ok...\n");
    arm32_icache_disable();
    printk(LOG_LEVEL_INFO, "disable icache ok...\n");
    arm32_interrupt_disable();
    printk(LOG_LEVEL_INFO, "free interrupt ok...\n");
    enable_kernel_smp();
    printk(LOG_LEVEL_INFO, "enable kernel smp ok...\n");

    printk(LOG_LEVEL_INFO, "jump to kernel address: 0x%x\n\n", image.dest);

    kernel_entry = (void (*)(int, int, unsigned int)) entry_point;
    kernel_entry(0, ~0, (unsigned int) image.of_dest);

    // if kernel boot not success, jump to fel.
    jmp_to_fel();

    return 0;
}
