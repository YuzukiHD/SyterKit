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

image_info_t image;

unsigned int code[9];
unsigned int magic;
unsigned int start;
unsigned int end;

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
