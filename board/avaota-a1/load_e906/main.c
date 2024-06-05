/* SPDX-License-Identifier: Apache-2.0 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <config.h>
#include <log.h>
#include <timer.h>

#include <common.h>
#include <jmp.h>
#include <mmu.h>
#include <smalloc.h>
#include <sstdlib.h>
#include <string.h>

#include <cli.h>
#include <cli_shell.h>
#include <cli_termesc.h>

#include <sys-clk.h>
#include <sys-dram.h>
#include <sys-i2c.h>
#include <sys-rtc.h>
#include <sys-sdcard.h>
#include <sys-sid.h>
#include <sys-spi.h>

#include <pmu/axp.h>

#include <fdt_wrapper.h>
#include <elf_loader.h>
#include <ff.h>
#include <sys-sdhci.h>
#include <uart.h>

#define CONFIG_E906_FILENAME "e906.bin"
#define CONFIG_E906_LOAD_ADDR (0x48100000)

#define CONFIG_SDMMC_SPEED_TEST_SIZE 1024// (unit: 512B sectors)

#define CONFIG_DEFAULT_BOOTDELAY 3

#define CONFIG_HEAP_BASE (0x40800000)
#define CONFIG_HEAP_SIZE (16 * 1024 * 1024)

extern sunxi_serial_t uart_dbg;

extern sunxi_i2c_t i2c_pmu;

extern sunxi_sdhci_t sdhci0;

extern uint32_t dram_para[32];

extern void enable_sram_a3();
extern void rtc_set_vccio_det_spare();
extern void set_rpio_power_mode(void);
extern void sunxi_nsi_init();

typedef struct atf_head {
    uint32_t jump_instruction; /* jumping to real code */
    uint8_t magic[8];          /* magic */
    uint32_t scp_base;         /* scp openrisc core bin */
    uint32_t next_boot_base;   /* next boot base for uboot */
    uint32_t nos_base;         /* ARM SVC RUNOS base */
    uint32_t secureos_base;    /* optee base */
    uint8_t version[8];        /* atf version */
    uint8_t platform[8];       /* platform information */
    uint32_t reserved[1];      /* stamp space, 16bytes align */
    uint32_t dram_para[32];    /* the dram param */
    uint64_t dtb_base;         /* the address of dtb */
} atf_head_t;

#define FILENAME_MAX_LEN 16
typedef struct {
    uint8_t *e906_dest;
    char e906_filename[FILENAME_MAX_LEN];
} image_info_t;

image_info_t image;

#define CHUNK_SIZE 0x20000

static int fatfs_loadimage(char *filename, BYTE *dest) {
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

    printk_info("FATFS: read in %ums at %.2fMB/S\n", time,
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
    sdmmc_blk_read(&card0, (uint8_t *) (SDRAM_BASE), 0, CONFIG_SDMMC_SPEED_TEST_SIZE);
    test_time = time_ms() - start;
    printk_debug("SDMMC: speedtest %uKB in %ums at %uKB/S\n",
                 (CONFIG_SDMMC_SPEED_TEST_SIZE * 512) / 1024, test_time,
                 (CONFIG_SDMMC_SPEED_TEST_SIZE * 512) / test_time);

    start = time_ms();

    fret = f_mount(&fs, "", 1);
    if (fret != FR_OK) {
        printk_error("FATFS: mount error: %d\n", fret);
        return -1;
    } else {
        printk_debug("FATFS: mount OK\n");
    }

    printk_info("FATFS: read %s addr=%x\n", image->e906_filename, (uint32_t) image->e906_dest);
    ret = fatfs_loadimage(image->e906_filename, image->e906_dest);
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

    show_banner();

    rtc_set_vccio_det_spare();

    sunxi_clk_init();

    set_rpio_power_mode();

    sunxi_clk_dump();

    sunxi_i2c_init(&i2c_pmu);

    pmu_axp2202_init(&i2c_pmu);

    pmu_axp1530_init(&i2c_pmu);

    pmu_axp2202_set_vol(&i2c_pmu, "dcdc1", 1100, 1);

    pmu_axp1530_set_dual_phase(&i2c_pmu);
    pmu_axp1530_set_vol(&i2c_pmu, "dcdc1", 1100, 1);
    pmu_axp1530_set_vol(&i2c_pmu, "dcdc2", 1100, 1);

    pmu_axp2202_set_vol(&i2c_pmu, "dcdc2", 920, 1);
    pmu_axp2202_set_vol(&i2c_pmu, "dcdc3", 1160, 1);
    pmu_axp2202_set_vol(&i2c_pmu, "dcdc4", 3300, 1);

    pmu_axp2202_dump(&i2c_pmu);
    pmu_axp1530_dump(&i2c_pmu);

    enable_sram_a3();

    /* Initialize the DRAM and enable memory management unit (MMU). */
    uint64_t dram_size = sunxi_dram_init(&dram_para);

    sunxi_clk_dump();

    arm32_mmu_enable(SDRAM_BASE, dram_size);

    /* Initialize the small memory allocator. */
    smalloc_init(CONFIG_HEAP_BASE, CONFIG_HEAP_SIZE);

    sunxi_nsi_init();

    /* Clear the image_info_t struct. */
    memset(&image, 0, sizeof(image_info_t));
    image.e906_dest = (uint8_t *) CONFIG_E906_LOAD_ADDR;
    strcpy(image.e906_filename, CONFIG_E906_FILENAME);

    /* Initialize the SD host controller. */
    if (sunxi_sdhci_init(&sdhci0) != 0) {
        printk_error("SMHC: %s controller init failed\n", sdhci0.name);
        goto _shell;
    } else {
        printk_info("SMHC: %s controller initialized\n", sdhci0.name);
    }

    /* Initialize the SD card and check if initialization is successful. */
    if (sdmmc_init(&card0, &sdhci0) != 0) {
        printk_warning("SMHC: init failed, Retrying...\n");
        mdelay(30);
        if (sdmmc_init(&card0, &sdhci0) != 0) {
            printk_warning("SMHC: init failed\n");
            goto _shell;
        }
    }

    /* Load the DTB, kernel image, and configuration data from the SD card. */
    if (load_sdcard(&image) != 0) {
        printk_warning("SMHC: loading failed\n");
        goto _shell;
    }

    sunxi_e906_clock_reset();

    /* E906 need to remap addresses for some addr. */
    vaddr_range_t e906_addr_mapping_range[] = {
            {0x3FFC0000, 0x4003FFFF, 0x07280000},
            {0x40400000, 0x7FFFFFFF, 0x40400000},
    };

    vaddr_map_t e906_addr_mapping = {
            .range = e906_addr_mapping_range,
            .range_size = sizeof(e906_addr_mapping_range) / sizeof(vaddr_range_t),
    };

    uint32_t elf_run_addr = elf32_get_entry_addr((phys_addr_t) image.e906_dest);
    printk_info("RISC-V ELF run addr: 0x%08x\n", elf_run_addr);

    if (load_elf32_image_remap((phys_addr_t) image.e906_dest, &e906_addr_mapping)) {
        printk_error("RISC-V ELF load FAIL\n");
    }

    sunxi_e906_clock_init(elf_run_addr);

    dump_e906_clock();

    printk_info("RISC-V E906 Core now Running... \n");

    abort();

_shell:
    syterkit_shell_attach(NULL);

    return 0;
}