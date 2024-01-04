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

#include "sys-dram.h"
#include "sys-i2c.h"
#include "sys-rtc.h"
#include "sys-sdcard.h"
#include "sys-sid.h"
#include "sys-spi.h"

#include "fdt_wrapper.h"
#include "ff.h"
#include "libfdt.h"
#include "uart.h"

#define CONFIG_KERNEL_FILENAME "zImage"
#define CONFIG_DTB_FILENAME "sunxi.dtb"
#define CONFIG_CONFIG_FILENAME "config.txt"

#define CONFIG_SDMMC_SPEED_TEST_SIZE 1024// (unit: 512B sectors)

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

    uint8_t *config_dest;
    uint8_t is_config;

    char filename[FILENAME_MAX_LEN];

    char of_filename[FILENAME_MAX_LEN];

    char config_filename[FILENAME_MAX_LEN];
} image_info_t;

#define MAX_SECTION_LEN 16
#define MAX_KEY_LEN 16
#define MAX_VALUE_LEN 512
#define CONFIG_MAX_ENTRY 3

typedef struct {
    char section[MAX_SECTION_LEN];
    char key[MAX_KEY_LEN];
    char value[MAX_VALUE_LEN];
} IniEntry;

IniEntry entries[CONFIG_MAX_ENTRY];

/* Linux zImage Header */
#define LINUX_ZIMAGE_MAGIC 0x016f2818
typedef struct {
    uint32_t code[9];
    uint32_t magic;
    uint32_t start;
    uint32_t end;
} linux_zimage_header_t;

extern sunxi_serial_t uart_dbg;

extern sunxi_i2c_t i2c_pmu;

extern sdhci_t sdhci0;

image_info_t image;

uint32_t code[9];
uint32_t magic;
uint32_t start;
uint32_t end;

static int boot_image_setup(uint8_t *addr, uint32_t *entry) {
    linux_zimage_header_t *zimage_header = (linux_zimage_header_t *) addr;
    printk(LOG_LEVEL_DEBUG, "Linux zImage->magic = 0x%x\n",
           zimage_header->magic);
    printk(LOG_LEVEL_DEBUG, "Linux zImage->start = 0x%x\n",
           (uint32_t) addr + zimage_header->start);
    printk(LOG_LEVEL_DEBUG, "Linux zImage->end   = 0x%x\n",
           (uint32_t) addr + zimage_header->end);

    if (zimage_header->magic == LINUX_ZIMAGE_MAGIC) {
        *entry = ((uint32_t) addr + zimage_header->start);
        return 0;
    }

    printk(LOG_LEVEL_ERROR, "unsupported kernel image\n");

    return -1;
}

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
        printk(LOG_LEVEL_ERROR, "FATFS: open, filename: [%s]: error %d\n", filename, fret);
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

    printk(LOG_LEVEL_INFO, "FATFS: read in %ums at %.2fMB/S\n", time,
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

    /* load DTB */
    printk(LOG_LEVEL_INFO, "FATFS: read %s addr=%x\n", image->of_filename, (uint32_t) image->of_dest);
    ret = fatfs_loadimage(image->of_filename, image->of_dest);
    if (ret)
        return ret;

    /* load Kernel */
    printk(LOG_LEVEL_INFO, "FATFS: read %s addr=%x\n", image->filename, (uint32_t) image->dest);
    ret = fatfs_loadimage(image->filename, image->dest);
    if (ret)
        return ret;

    /* load config */
    printk(LOG_LEVEL_INFO, "FATFS: read %s addr=%x\n", image->config_filename, (uint32_t) image->config_dest);
    ret = fatfs_loadimage(image->config_filename, image->config_dest);
    if (ret) {
        printk(LOG_LEVEL_INFO, "CONFIG: Cannot find config file, Using default config.\n");
        image->is_config = 0;
    } else {
        image->is_config = 1;
    }

    /* umount fs */
    fret = f_mount(0, "", 0);
    if (fret != FR_OK) {
        printk(LOG_LEVEL_ERROR, "FATFS: unmount error %d\n", fret);
        return -1;
    } else {
        printk(LOG_LEVEL_DEBUG, "FATFS: unmount OK\n");
    }
    printk(LOG_LEVEL_INFO, "FATFS: done in %ums\n", time_ms() - start);

    return 0;
}

static int abortboot_single_key(int bootdelay) {
    int abort = 0;
    unsigned long ts;

    printk(LOG_LEVEL_INFO, "Hit any key to stop autoboot: %2d ", bootdelay);

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

static void set_pmu_fin_voltage(char *power_name, uint32_t voltage) {
    int set_vol = voltage;
    int temp_vol, src_vol = pmu_axp1530_get_vol(&i2c_pmu, power_name);
    if (src_vol > voltage) {
        for (temp_vol = src_vol; temp_vol >= voltage; temp_vol -= 50) {
            pmu_axp1530_set_vol(&i2c_pmu, power_name, temp_vol, 1);
        }
    } else if (src_vol < voltage) {
        for (temp_vol = src_vol; temp_vol <= voltage; temp_vol += 50) {
            pmu_axp1530_set_vol(&i2c_pmu, power_name, temp_vol, 1);
        }
    }
    mdelay(30); /* Delay 300ms for pmu bootup */
}

msh_declare_command(reload);
msh_define_help(reload, "rescan TF Card and reload DTB, Kernel zImage", "Usage: reload\n");
int cmd_reload(int argc, const char **argv) {
    if (sdmmc_init(&card0, &sdhci0) != 0) {
        printk(LOG_LEVEL_ERROR, "SMHC: init failed\n");
        return 0;
    }

    if (load_sdcard(&image) != 0) {
        printk(LOG_LEVEL_ERROR, "SMHC: loading failed\n");
        return 0;
    }
    return 0;
}

msh_declare_command(boot);
msh_define_help(boot, "boot to linux", "Usage: boot\n");
int cmd_boot(int argc, const char **argv) {
    /* Initialize variables for kernel entry point and SD card access. */
    uint32_t entry_point = 0;
    void (*kernel_entry)(int zero, int arch, uint32_t params);

    /* Set up boot parameters for the kernel. */
    if (boot_image_setup((uint8_t *) image.dest, &entry_point)) {
        printk(LOG_LEVEL_ERROR, "boot setup failed\n");
        abort();
    }

    /* Disable MMU, data cache, instruction cache, interrupts */
    clean_syterkit_data();
    /* Debug message to indicate the kernel address that the system is jumping to. */
    printk(LOG_LEVEL_INFO, "jump to kernel address: 0x%x\n\n", image.dest);

    /* Jump to the kernel entry point. */
    kernel_entry = (void (*)(int, int, uint32_t)) entry_point;
    kernel_entry(0, ~0, (uint32_t) image.of_dest);
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
int main(void) {
    /* Initialize the debug serial interface. */
    sunxi_serial_init(&uart_dbg);

    /* Display the bootloader banner. */
    show_banner();

	/* enbale rtc vccio detected */
    rtc_set_vccio_det_spare();

    /* Initialize the system clock. */
    sunxi_clk_init();

    /* Check rtc fel flag. if set flag, goto fel */
    if (rtc_probe_fel_flag()) {
        printk(LOG_LEVEL_INFO, "RTC: get fel flag, jump to fel mode.\n");
        clean_syterkit_data();
        rtc_clear_fel_flag();
        sunxi_clk_reset();
        mdelay(100);
        goto _fel;
    }

    set_rpio_power_mode();
    
    sunxi_i2c_init(&i2c_pmu);

    pmu_axp1530_init(&i2c_pmu);
    set_pmu_fin_voltage("dcdc2", 1100);
    set_pmu_fin_voltage("dcdc3", 1100);

    /* Initialize the DRAM and enable memory management unit (MMU). */
    uint64_t dram_size = sunxi_dram_init(NULL);
    arm32_mmu_enable(SDRAM_BASE, dram_size);

    /* Initialize the small memory allocator. */
    smalloc_init(CONFIG_HEAP_BASE, CONFIG_HEAP_SIZE);

    /* Clear the image_info_t struct. */
    memset(&image, 0, sizeof(image_info_t));

    /* Set the destination address for the device tree binary (DTB), kernel image, and configuration data. */
    image.of_dest = (uint8_t *) CONFIG_DTB_LOAD_ADDR;
    image.dest = (uint8_t *) CONFIG_KERNEL_LOAD_ADDR;
    image.config_dest = (uint8_t *) CONFIG_CONFIG_LOAD_ADDR;
    image.is_config = 0;

    /* Copy the filenames for the DTB, kernel image, and configuration data. */
    strcpy(image.filename, CONFIG_KERNEL_FILENAME);
    strcpy(image.of_filename, CONFIG_DTB_FILENAME);
    strcpy(image.config_filename, CONFIG_CONFIG_FILENAME);

    /* Initialize the SD host controller. */
    if (sunxi_sdhci_init(&sdhci0) != 0) {
        printk(LOG_LEVEL_ERROR, "SMHC: %s controller init failed\n", sdhci0.name);
        goto _shell;
    } else {
        printk(LOG_LEVEL_INFO, "SMHC: %s controller initialized\n", sdhci0.name);
    }

    /* Initialize the SD card and check if initialization is successful. */
    if (sdmmc_init(&card0, &sdhci0) != 0) {
        printk(LOG_LEVEL_WARNING, "SMHC: init failed\n");
        goto _shell;
    }

    /* Load the DTB, kernel image, and configuration data from the SD card. */
    if (load_sdcard(&image) != 0) {
        printk(LOG_LEVEL_WARNING, "SMHC: loading failed\n");
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
