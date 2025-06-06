/* SPDX-License-Identifier: GPL-2.0+ */

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

#include "sys-dram.h"
#include "sys-i2c.h"
#include "sys-rtc.h"
#include "sys-sdcard.h"
#include "sys-sid.h"
#include "sys-spi.h"

#include "fdt_wrapper.h"
#include "ff.h"
#include "libfdt.h"
#include "sys-sdhci.h"
#include "uart.h"

#define CONFIG_BL31_FILENAME "bl31.bin"
#define CONFIG_BL31_LOAD_ADDR (0x48000000)

#define CONFIG_DTB_FILENAME "sun50i-r818-mg-core.dtb"
#define CONFIG_DTB_LOAD_ADDR (0x41f00000)

#define CONFIG_KERNEL_FILENAME "Image"
#define CONFIG_KERNEL_LOAD_ADDR (0x40080000)

#define CONFIG_SDMMC_SPEED_TEST_SIZE 1024// (unit: 512B sectors)

#define CONFIG_DEFAULT_BOOTDELAY 3

#define CONFIG_HEAP_BASE (0x40800000)
#define CONFIG_HEAP_SIZE (16 * 1024 * 1024)

extern sunxi_serial_t uart_dbg;
extern sunxi_i2c_t i2c_pmu;
extern sdhci_t sdhci0;
extern uint32_t dram_para[32];

extern int ar100s_gpu_fix(void);

typedef struct atf_head {
    uint32_t jump_instruction; /* jumping to real code */
    uint8_t magic[8];          /* ="u-boot" */
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

#define FILENAME_MAX_LEN 32
typedef struct {
    uint8_t *bl31_dest;
    char bl31_filename[FILENAME_MAX_LEN];

    uint8_t *kernel_dest;
    char kernel_filename[FILENAME_MAX_LEN];

    uint8_t *of_dest;
    char of_filename[FILENAME_MAX_LEN];

    uint8_t *config_dest;
    uint8_t is_config;
    char config_filename[FILENAME_MAX_LEN];
} image_info_t;

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

    printk_debug("FATFS: read in %ums at %.2fMB/S\n", time,
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
    printk_debug("SDMMC: speedtest %uKB in %ums at %uKB/S\n", (CONFIG_SDMMC_SPEED_TEST_SIZE * 512) / 1024, test_time, (CONFIG_SDMMC_SPEED_TEST_SIZE * 512) / test_time);

    start = time_ms();

    mdelay(10);

    fret = f_mount(&fs, "", 1);
    if (fret != FR_OK) {
        printk_error("FATFS: mount error: %d\n", fret);
        return -1;
    } else {
        printk_debug("FATFS: mount OK\n");
    }

    printk_info("FATFS: read %s addr=%x\n", image->bl31_filename, (uint32_t) image->bl31_dest);
    ret = fatfs_loadimage(image->bl31_filename, image->bl31_dest);
    if (ret)
        return ret;

    printk_info("FATFS: read %s addr=%x\n", image->of_filename, (uint32_t) image->of_dest);
    ret = fatfs_loadimage(image->of_filename, image->of_dest);
    if (ret)
        return ret;

    printk_info("FATFS: read %s addr=%x\n", image->kernel_filename, (uint32_t) image->kernel_dest);
    ret = fatfs_loadimage(image->kernel_filename, image->kernel_dest);
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

void jmp_to_arm64(uint32_t addr) {
    /* Set RTC data to current time_ms(), Save in RTC_FEL_INDEX */
    rtc_set_start_time_ms();

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

static void set_pmu_fin_voltage(char *power_name, uint32_t voltage) {
    int set_vol = voltage;
    int temp_vol, src_vol = pmu_axp2202_get_vol(&i2c_pmu, power_name);
    if (src_vol > voltage) {
        for (temp_vol = src_vol; temp_vol >= voltage; temp_vol -= 50) {
            pmu_axp2202_set_vol(&i2c_pmu, power_name, temp_vol, 1);
        }
    } else if (src_vol < voltage) {
        for (temp_vol = src_vol; temp_vol <= voltage; temp_vol += 50) {
            pmu_axp2202_set_vol(&i2c_pmu, power_name, temp_vol, 1);
        }
    }
    mdelay(30); /* Delay 300ms for pmu bootup */
}

static int abortboot_single_key(int bootdelay) {
    int abort = 0;
    unsigned long ts;

    printk_info("Hit any key to stop autoboot: %2d ", bootdelay);

    /* Check if key already pressed */
    if (tstc()) {       /* we got a key press */
        uart_getchar(); /* consume input */
        printk(LOG_LEVEL_MUTE, "\b\b\b%2d", bootdelay);
        abort = 1; /* don't auto boot */
    }

    while ((bootdelay > 0) && (!abort)) {
        --bootdelay;
        /* delay 1000 ms */
        ts = time_ms();
        do {
            if (tstc()) {  /* we got a key press */
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
int cmd_boot(int argc, const char **argv) {
    atf_head_t *atf_head = (atf_head_t *) image.bl31_dest;

    atf_head->nos_base = CONFIG_KERNEL_LOAD_ADDR;
    atf_head->dtb_base = CONFIG_DTB_LOAD_ADDR;

    printk_info("ATF: Kernel addr: 0x%08x\n", atf_head->nos_base);
    printk_info("ATF: Kernel DTB addr: 0x%08x\n", atf_head->dtb_base);

    clean_syterkit_data();

    jmp_to_arm64(CONFIG_BL31_LOAD_ADDR);

    printk_info("Back to SyterKit\n");

    // if kernel boot not success, jump to fel.
    jmp_to_fel();
    return 0;
}

const msh_command_entry commands[] = {
        msh_define_command(boot),
        msh_command_end,
};

int main(void) {
    sunxi_serial_init(&uart_dbg);

    printk_info("Checking SoC Version...\n");

    ar100s_gpu_fix();

    show_banner();

    sunxi_clk_init();

    sunxi_clk_dump();

    sunxi_i2c_init(&i2c_pmu);

    pmu_axp2202_init(&i2c_pmu);

    set_pmu_fin_voltage("dcdc1", 1100);
    set_pmu_fin_voltage("dcdc3", 1100);

    pmu_axp2202_dump(&i2c_pmu);

    /* Initialize the DRAM and enable memory management unit (MMU). */
    uint32_t dram_size = sunxi_dram_init(&dram_para);

    arm32_mmu_enable(SDRAM_BASE, dram_size);

    /* Initialize the small memory allocator. */
    smalloc_init(CONFIG_HEAP_BASE, CONFIG_HEAP_SIZE);

    /* Clear the image_info_t struct. */
    memset(&image, 0, sizeof(image_info_t));

    image.bl31_dest = (uint8_t *) CONFIG_BL31_LOAD_ADDR;
    image.of_dest = (uint8_t *) CONFIG_DTB_LOAD_ADDR;
    image.kernel_dest = (uint8_t *) CONFIG_KERNEL_LOAD_ADDR;

    strcpy(image.bl31_filename, CONFIG_BL31_FILENAME);
    strcpy(image.of_filename, CONFIG_DTB_FILENAME);
    strcpy(image.kernel_filename, CONFIG_KERNEL_FILENAME);
    /* Initialize the SD host controller. */
    if (sunxi_sdhci_init(&sdhci0) != 0) {
        printk_error("SMHC: %s controller init failed\n", sdhci0.name);
        goto _shell;
    } else {
        printk_info("SMHC: %s controller initialized\n", sdhci0.name);
    }

    /* Initialize the SD card and check if initialization is successful. */
    if (sdmmc_init(&card0, &sdhci0) != 0) {
        printk_warning("SMHC: init failed\n");
        goto _shell;
    }

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

    return 0;
}