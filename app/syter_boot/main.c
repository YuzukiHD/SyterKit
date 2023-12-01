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
#include "fdt.h"

extern uint32_t _start;
extern uint32_t __spl_start;
extern uint32_t __spl_end;
extern uint32_t __spl_size;
extern uint32_t __stack_srv_start;
extern uint32_t __stack_srv_end;
extern uint32_t __stack_ddr_srv_start;
extern uint32_t __stack_ddr_srv_end;

#define CONFIG_KERNEL_FILENAME "zImage"
#define CONFIG_DTB_FILENAME "sun8i-v851s-tinyvision.dtb"

#define CONFIG_SDMMC_SPEED_TEST_SIZE 1024 // (unit: 512B sectors)

#define CONFIG_DTB_LOAD_ADDR    (0x41008000)
#define CONFIG_KERNEL_LOAD_ADDR (0x41800000)

// 128KB erase sectors, so place them starting from 2nd sector
#define CONFIG_SPINAND_DTB_ADDR (128 * 2048)
#define CONFIG_SPINAND_KERNEL_ADDR (256 * 2048)

#define FILENAME_MAX_LEN 64
typedef struct
{
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
typedef struct
{
    unsigned int code[9];
    unsigned int magic;
    unsigned int start;
    unsigned int end;
} linux_zimage_header_t;

sunxi_uart_t uart_dbg = {
    .base = 0x02500000,
    .id = 2,
    .gpio_tx = {GPIO_PIN(PORTH, 9), GPIO_PERIPH_MUX5},
    .gpio_rx = {GPIO_PIN(PORTH, 10), GPIO_PERIPH_MUX5},
};

sunxi_spi_t sunxi_spi0 = {
    .base = 0x04025000,
    .id = 0,
    .clk_rate = 75 * 1000 * 1000,
    .gpio_cs = {GPIO_PIN(PORTC, 1), GPIO_PERIPH_MUX4},
    .gpio_sck = {GPIO_PIN(PORTC, 0), GPIO_PERIPH_MUX4},
    .gpio_mosi = {GPIO_PIN(PORTC, 2), GPIO_PERIPH_MUX4},
    .gpio_miso = {GPIO_PIN(PORTC, 3), GPIO_PERIPH_MUX4},
    .gpio_wp = {GPIO_PIN(PORTC, 4), GPIO_PERIPH_MUX4},
    .gpio_hold = {GPIO_PIN(PORTC, 5), GPIO_PERIPH_MUX4},
};

sdhci_t sdhci0 = {
    .name = "sdhci0",
    .reg = (sdhci_reg_t *)0x04020000,
    .voltage = MMC_VDD_27_36,
    .width = MMC_BUS_WIDTH_4,
    .clock = MMC_CLK_50M,
    .removable = 0,
    .isspi = FALSE,
    .gpio_clk = {GPIO_PIN(PORTF, 2), GPIO_PERIPH_MUX2},
    .gpio_cmd = {GPIO_PIN(PORTF, 3), GPIO_PERIPH_MUX2},
    .gpio_d0 = {GPIO_PIN(PORTF, 1), GPIO_PERIPH_MUX2},
    .gpio_d1 = {GPIO_PIN(PORTF, 0), GPIO_PERIPH_MUX2},
    .gpio_d2 = {GPIO_PIN(PORTF, 5), GPIO_PERIPH_MUX2},
    .gpio_d3 = {GPIO_PIN(PORTF, 4), GPIO_PERIPH_MUX2},
};

image_info_t image;

    unsigned int code[9];
    unsigned int magic;
    unsigned int start;
    unsigned int end;

static int boot_image_setup(unsigned char *addr, unsigned int *entry)
{
    linux_zimage_header_t *zimage_header = (linux_zimage_header_t *)addr;

    printk(LOG_LEVEL_DEBUG, "Linux zImage->code  = 0x");
    for(int i = 0; i < 9; i++)
        printk(LOG_LEVEL_MUTE, "%x", code[i]);

    printk(LOG_LEVEL_MUTE, "\r\n");
    printk(LOG_LEVEL_DEBUG, "Linux zImage->magic = 0x%x\r\n", zimage_header->magic);
    printk(LOG_LEVEL_DEBUG, "Linux zImage->start = 0x%x\r\n", (unsigned int)addr + zimage_header->start);
    printk(LOG_LEVEL_DEBUG, "Linux zImage->end   = 0x%x\r\n", (unsigned int)addr + zimage_header->end);

    if (zimage_header->magic == LINUX_ZIMAGE_MAGIC)
    {
        *entry = ((unsigned int)addr + zimage_header->start);
        return 0;
    }

    printk(LOG_LEVEL_ERROR, "unsupported kernel image\r\n");

    return -1;
}

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
    if (fret != FR_OK)
    {
        printk(LOG_LEVEL_ERROR, "FATFS: open, filename: [%s]: error %d\r\n", filename, fret);
        ret = -1;
        goto open_fail;
    }

    start = time_ms();

    do
    {
        byte_read = 0;
        fret = f_read(&file, (void *)(dest), byte_to_read, &byte_read);
        dest += byte_to_read;
        total_read += byte_read;
    } while (byte_read >= byte_to_read && fret == FR_OK);

    time = time_ms() - start + 1;

    if (fret != FR_OK)
    {
        printk(LOG_LEVEL_ERROR, "FATFS: read: error %d\r\n", fret);
        ret = -1;
        goto read_fail;
    }
    ret = 0;

read_fail:
    fret = f_close(&file);

    printk(LOG_LEVEL_DEBUG, "FATFS: read in %ums at %.2fMB/S\r\n", time, (f32)(total_read / time) / 1024.0f);

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
    sdmmc_blk_read(&card0, (uint8_t *)(SDRAM_BASE), 0, CONFIG_SDMMC_SPEED_TEST_SIZE);
    test_time = time_ms() - start;
    printk(LOG_LEVEL_DEBUG, "SDMMC: speedtest %uKB in %ums at %uKB/S\r\n", (CONFIG_SDMMC_SPEED_TEST_SIZE * 512) / 1024, test_time,
           (CONFIG_SDMMC_SPEED_TEST_SIZE * 512) / test_time);

    start = time_ms();

    fret = f_mount(&fs, "", 1);
    if (fret != FR_OK)
    {
        printk(LOG_LEVEL_ERROR, "FATFS: mount error: %d\r\n", fret);
        return -1;
    }
    else
    {
        printk(LOG_LEVEL_DEBUG, "FATFS: mount OK\r\n");
    }

    printk(LOG_LEVEL_INFO, "FATFS: read %s addr=%x\r\n", image->of_filename, (unsigned int)image->of_dest);
    ret = fatfs_loadimage(image->of_filename, image->of_dest);
    if (ret)
        return ret;

    printk(LOG_LEVEL_INFO, "FATFS: read %s addr=%x\r\n", image->filename, (unsigned int)image->dest);
    ret = fatfs_loadimage(image->filename, image->dest);
    if (ret)
        return ret;

    /* umount fs */
    fret = f_mount(0, "", 0);
    if (fret != FR_OK)
    {
        printk(LOG_LEVEL_ERROR, "FATFS: unmount error %d\r\n", fret);
        return -1;
    }
    else
    {
        printk(LOG_LEVEL_DEBUG, "FATFS: unmount OK\r\n");
    }
    printk(LOG_LEVEL_DEBUG, "FATFS: done in %ums\r\n", time_ms() - start);

    return 0;
}

int load_spi_nand(sunxi_spi_t *spi, image_info_t *image)
{
    linux_zimage_header_t *hdr;
    unsigned int size;
    uint64_t start, time;

    if (spi_nand_detect(spi) != 0)
        return -1;

    /* get dtb size and read */
    spi_nand_read(spi, image->of_dest, CONFIG_SPINAND_DTB_ADDR, (uint32_t)sizeof(boot_param_header_t));
    if (of_get_magic_number(image->of_dest) != OF_DT_MAGIC)
    {
        printk(LOG_LEVEL_ERROR, "SPI-NAND: DTB verification failed\r\n");
        return -1;
    }

    size = of_get_dt_total_size(image->of_dest);
    printk(LOG_LEVEL_DEBUG, "SPI-NAND: dt blob: Copy from 0x%08x to 0x%08lx size:0x%08x\r\n", CONFIG_SPINAND_DTB_ADDR,
           (uint32_t)image->of_dest, size);
    start = time_us();
    spi_nand_read(spi, image->of_dest, CONFIG_SPINAND_DTB_ADDR, (uint32_t)size);
    time = time_us() - start;
    printk(LOG_LEVEL_INFO, "SPI-NAND: read dt blob of size %u at %.2fMB/S\r\n", size, (f32)(size / time));

    /* get kernel size and read */
    spi_nand_read(spi, image->dest, CONFIG_SPINAND_KERNEL_ADDR, (uint32_t)sizeof(linux_zimage_header_t));
    hdr = (linux_zimage_header_t *)image->dest;
    if (hdr->magic != LINUX_ZIMAGE_MAGIC)
    {
        printk(LOG_LEVEL_DEBUG, "SPI-NAND: zImage verification failed\r\n");
        return -1;
    }
    size = hdr->end - hdr->start;
    printk(LOG_LEVEL_DEBUG, "SPI-NAND: Image: Copy from 0x%08x to 0x%08lx size:0x%08x\r\n", CONFIG_SPINAND_KERNEL_ADDR,
           (uint32_t)image->dest, size);
    start = time_us();
    spi_nand_read(spi, image->dest, CONFIG_SPINAND_KERNEL_ADDR, (uint32_t)size);
    time = time_us() - start;
    printk(LOG_LEVEL_INFO, "SPI-NAND: read Image of size %u at %.2fMB/S\r\n", size, (f32)(size / time));

    return 0;
}

void abort(void)
{
    while (1)
        ;
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
    printk(LOG_LEVEL_INFO, " %s V0.1.1 Commit: %s\r\n", PROJECT_NAME, PROJECT_GIT_HASH);
    printk(LOG_LEVEL_INFO, "***********************************\r\n");

	id[0] = read32(0x03006200 + 0x0);
	id[1] = read32(0x03006200 + 0x4);
	id[2] = read32(0x03006200 + 0x8);
	id[3] = read32(0x03006200 + 0xc);
    
	printk(LOG_LEVEL_INFO, "Chip ID is: %08x%08x%08x%08x\r\n", id[0], id[1], id[2], id[3]);
}

int main(void)
{
    sunxi_uart_init(&uart_dbg);

    show_banner();

    sunxi_clk_init();

    sunxi_dram_init();

    unsigned int entry_point = 0;
    void (*kernel_entry)(int zero, int arch, unsigned int params);

    sunxi_clk_dump();

    memset(&image, 0, sizeof(image_info_t));

    image.of_dest = (uint8_t *)CONFIG_DTB_LOAD_ADDR;
    image.dest = (uint8_t *)CONFIG_KERNEL_LOAD_ADDR;

    strcpy(image.filename, CONFIG_KERNEL_FILENAME);
    strcpy(image.of_filename, CONFIG_DTB_FILENAME);

    if (sunxi_sdhci_init(&sdhci0) != 0)
    {
        printk(LOG_LEVEL_ERROR, "SMHC: %s controller init failed\r\n", sdhci0.name);
    }
    else
    {
        printk(LOG_LEVEL_INFO, "SMHC: %s controller v%x initialized\r\n", sdhci0.name, sdhci0.reg->vers);
    }
    if (sdmmc_init(&card0, &sdhci0) != 0)
    {
        printk(LOG_LEVEL_WARNING, "SMHC: init failed, trying SPI\r\n");
        goto _spi;
    }

    if (load_sdcard(&image) != 0)
    {
        printk(LOG_LEVEL_WARNING, "SMHC: loading failed, trying SPI\r\n");
    }
    else
    {
        goto _boot;
    }

_spi:
    dma_init();
    dma_test((uint32_t *)CONFIG_DTB_LOAD_ADDR, (uint32_t *)CONFIG_KERNEL_LOAD_ADDR);
    printk(LOG_LEVEL_DEBUG, "SPI: init\r\n");
    if (sunxi_spi_init(&sunxi_spi0) != 0)
    {
        printk(LOG_LEVEL_ERROR, "SPI: init failed\r\n");
    }

    if (load_spi_nand(&sunxi_spi0, &image) != 0)
    {
        printk(LOG_LEVEL_ERROR, "SPI-NAND: loading failed\r\n");
    }

    sunxi_spi_disable(&sunxi_spi0);
    dma_exit();

_boot:
    if (boot_image_setup((unsigned char *)image.dest, &entry_point))
    {
        printk(LOG_LEVEL_ERROR, "boot setup failed\r\n");
    }

    printk(LOG_LEVEL_INFO, "booting linux...\r\n");

    arm32_mmu_disable();
    printk(LOG_LEVEL_INFO, "disable mmu ok...\r\n");
    arm32_dcache_disable();
    printk(LOG_LEVEL_INFO, "disable dcache ok...\r\n");
    arm32_icache_disable();
    printk(LOG_LEVEL_INFO, "disable icache ok...\r\n");
    arm32_interrupt_disable();
    printk(LOG_LEVEL_INFO, "free interrupt ok...\r\n");
    enable_kernel_smp();
    printk(LOG_LEVEL_INFO, "enable kernel smp ok...\r\n");

    printk(LOG_LEVEL_INFO, "jump to kernel address: 0x%x\r\n", image.dest);

    kernel_entry = (void (*)(int, int, unsigned int))entry_point;
    kernel_entry(0, ~0, (unsigned int)image.of_dest);

    return 0;
}
