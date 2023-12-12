/* SPDX-License-Identifier: Apache-2.0 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <config.h>
#include <log.h>

#include <arm32.h>
#include <common.h>
#include <jmp.h>
#include <smalloc.h>
#include <string.h>

#include "sys-dram.h"
#include "sys-sdcard.h"
#include "sys-sid.h"
#include "sys-spi.h"
#include "sys-timer.h"

#include "fdt_wrapper.h"
#include "ff.h"
#include "libfdt.h"

#define CONFIG_KERNEL_FILENAME "zImage"
#define CONFIG_DTB_FILENAME "sunxi.dtb"
#define CONFIG_CONFIG_FILENAME "config.ini"

#define CONFIG_SDMMC_SPEED_TEST_SIZE 1024// (unit: 512B sectors)

#define CONFIG_DTB_LOAD_ADDR (0x41008000)
#define CONFIG_KERNEL_LOAD_ADDR (0x41800000)
#define CONFIG_CONFIG_LOAD_ADDR (0x40008000)
#define CONFIG_HEAP_BASE (0x40800000)
#define CONFIG_HEAP_SIZE (16 * 1024 * 1024)

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
#define MAX_ENTRY 2

typedef struct {
    char section[MAX_SECTION_LEN];
    char key[MAX_KEY_LEN];
    char value[MAX_VALUE_LEN];
} IniEntry;

IniEntry entries[MAX_ENTRY];

/* Linux zImage Header */
#define LINUX_ZIMAGE_MAGIC 0x016f2818
typedef struct {
    uint32_t code[9];
    uint32_t magic;
    uint32_t start;
    uint32_t end;
} linux_zimage_header_t;

sunxi_serial_t uart_dbg = {
        .base = 0x02500000,
        .id = 0,
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
        .reg = (sdhci_reg_t *) 0x04020000,
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

uint32_t code[9];
uint32_t magic;
uint32_t start;
uint32_t end;

static int boot_image_setup(uint8_t *addr, uint32_t *entry) {
    linux_zimage_header_t *zimage_header = (linux_zimage_header_t *) addr;

    printk(LOG_LEVEL_INFO, "Linux zImage->code  = 0x");
    for (int i = 0; i < 9; i++)
        printk(LOG_LEVEL_MUTE, "%x", code[i]);

    printk(LOG_LEVEL_MUTE, "\n");
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
    printk(LOG_LEVEL_DEBUG, "FATFS: done in %ums\n", time_ms() - start);

    return 0;
}

static void trim(char *str) {
    int len = strlen(str);
    while (len > 0 && (str[len - 1] == ' ' || str[len - 1] == '\n' || str[len - 1] == '\r')) {
        str[--len] = '\0';
    }
    while (*str && (*str == ' ' || *str == '\n' || *str == '\r')) {
        ++str;
        --len;
    }
}

static int parse_ini_data(const char *data, size_t size, IniEntry *entries, int max_entries) {
    char line[MAX_VALUE_LEN];
    char current_section[MAX_SECTION_LEN] = "";
    int entry_count = 0;

    const char *p = data;
    const char *end = data + size;

    while (p < end) {
        /* Read a line of data */
        size_t len = 0;
        while (p + len < end && *(p + len) != '\n') {
            ++len;
        }
        if (p + len < end && *(p + len) == '\n') {
            ++len;
        }
        if (len > 0) {
            strncpy(line, p, len);
            line[len] = '\0';
            p += len;

            trim(line);

            /* Ignore empty lines and comments */
            if (line[0] == '\0' || line[0] == ';' || line[0] == '#') {
                continue;
            }

            /* Parse the section name */
            if (line[0] == '[' && line[strlen(line) - 1] == ']') {
                strncpy(current_section, &line[1], strlen(line) - 2);
                current_section[strlen(line) - 2] = '\0';
                continue;
            }

            /* Parse key-value pairs */
            char *pos = strchr(line, '=');
            if (pos != NULL) {
                char *key_start = line;
                char *value_start = pos + 1;
                *pos = '\0';
                trim(key_start);
                trim(value_start);

                if (strlen(current_section) > 0 && strlen(key_start) > 0 && strlen(value_start) > 0) {
                    if (entry_count >= max_entries) {
                        printk(LOG_LEVEL_ERROR, "INI: Too many entries!\n");
                        break;
                    }
                    strncpy(entries[entry_count].section, current_section, MAX_SECTION_LEN - 1);
                    strncpy(entries[entry_count].key, key_start, MAX_KEY_LEN - 1);
                    strncpy(entries[entry_count].value, value_start, MAX_VALUE_LEN - 1);
                    ++entry_count;
                }
            }
        }
    }

    return entry_count;
}

static const char *find_entry_value(const IniEntry *entries, int entry_count, const char *section, const char *key) {
    for (int i = 0; i < entry_count; ++i) {
        if (strcmp(entries[i].section, section) == 0 && strcmp(entries[i].key, key) == 0) {
            return entries[i].value;
        }
    }
    return NULL;
}

static int update_bootargs_from_config(uint64_t dram_size) {
    int ret = 0;
    char *bootargs_str_config = NULL;

    /* Check if using config file, get bootargs in the config file */
    if (image.is_config) {
        size_t size_a = strlen(image.config_dest);
        int entry_count = parse_ini_data(image.config_dest, size_a, entries, MAX_ENTRY);
        for (int i = 0; i < entry_count; ++i) {
            /* Print parsed INI entries */
            printk(LOG_LEVEL_DEBUG, "INI: [%s] %s = %s\n", entries[i].section, entries[i].key, entries[i].value);
        }
        bootargs_str_config = find_entry_value(entries, entry_count, "configs", "bootargs");
    }

    /* Force image.dest to be a pointer to fdt_header structure */
    struct fdt_header *dtb_header = (struct fdt_header *) image.of_dest;

    /* Check if DTB header is valid */
    if ((ret = fdt_check_header(dtb_header)) != 0) {
        printk(LOG_LEVEL_ERROR, "Invalid device tree blob: %s\n", fdt_strerror(ret));
        abort();
    }

    /* Get the total size of DTB */
    uint32_t size = fdt_totalsize(image.of_dest);
    printk(LOG_LEVEL_DEBUG, "%s: FDT Size = %d\n", image.of_filename, size);

    int len = 0;
    /* Get the offset of "/chosen" node */
    uint32_t bootargs_node = fdt_path_offset(image.of_dest, "/chosen");

    /* Get bootargs string */
    char *bootargs_str = (void *) fdt_getprop(image.of_dest, bootargs_node, "bootargs", &len);

    /* If config file read fail or not using */
    if (bootargs_str_config == NULL) {
        printk(LOG_LEVEL_WARNING, "INI: Cannot parse bootargs, using default bootargs in DTB.\n");
        bootargs_str_config = bootargs_str;
    }

    /* Add dram size to dtb */
    char dram_size_str[8];
    strcat(bootargs_str_config, " mem=");
    strcat(bootargs_str_config, simple_ltoa(dram_size, dram_size_str, 10));
    strcat(bootargs_str_config, "M");

    /* Set bootargs based on the configuration file */
    printk(LOG_LEVEL_DEBUG, "INI: Set bootargs to %s\n", bootargs_str_config);

_add_dts_size:
    /* Modify bootargs string */
    ret = fdt_setprop(image.of_dest, bootargs_node, "bootargs", bootargs_str_config, strlen(bootargs_str_config) + 1);
    if (ret == -FDT_ERR_NOSPACE) {
        printk(LOG_LEVEL_DEBUG, "FDT: FDT_ERR_NOSPACE, Size = %d, Increase Size = %d\n", size, 512);
        ret = fdt_increase_size(image.of_dest, 512);
        if (!ret)
            goto _add_dts_size;
        else
            goto _err_size;
    } else if (ret < 0) {
        printk(LOG_LEVEL_ERROR, "Can't change bootargs node: %s\n", fdt_strerror(ret));
        abort();
    }

    /* Get the total size of DTB */
    printk(LOG_LEVEL_DEBUG, "Modify FDT Size = %d\n", fdt_totalsize(image.of_dest));

    if (ret < 0) {
        printk(LOG_LEVEL_ERROR, "libfdt fdt_setprop() error: %s\n", fdt_strerror(ret));
        abort();
    }

    return 0;
_err_size:
    printk(LOG_LEVEL_ERROR, "DTB: Can't increase blob size: %s\n", fdt_strerror(ret));
    abort();
}


/* 
 * main function for the bootloader. Initializes and sets up the system, loads the kernel and device tree binary from
 * an SD card, sets boot arguments, and boots the kernel. If the kernel fails to boot, the function jumps to FEL mode.
 */
int main(void) {
    /* Initialize the debug serial interface. */
    sunxi_serial_init(&uart_dbg);

    /* Display the bootloader banner. */
    show_banner();

    /* Initialize the system clock. */
    sunxi_clk_init();

    /* Initialize the DRAM and enable memory management unit (MMU). */
    uint64_t dram_size = sunxi_dram_init();
    arm32_mmu_enable(SDRAM_BASE, dram_size);

    /* Debug message to indicate that MMU is enabled. */
    printk(LOG_LEVEL_DEBUG, "enable mmu ok\n");

    /* Initialize the small memory allocator. */
    smalloc_init(CONFIG_HEAP_BASE, CONFIG_HEAP_SIZE);

    /* Set up Real-Time Clock (RTC) hardware. */
    rtc_set_vccio_det_spare();

    /* Check if system voltage is within limits. */
    sys_ldo_check();

    /* Initialize variables for kernel entry point and SD card access. */
    uint32_t entry_point = 0;
    void (*kernel_entry)(int zero, int arch, uint32_t params);

    /* Dump information about the system clocks. */
    sunxi_clk_dump();

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
    } else {
        printk(LOG_LEVEL_INFO, "SMHC: %s controller v%x initialized\n", sdhci0.name, sdhci0.reg->vers);
    }

    /* Initialize the SD card and check if initialization is successful. */
    if (sdmmc_init(&card0, &sdhci0) != 0) {
        printk(LOG_LEVEL_WARNING, "SMHC: init failed\n");
    }

    /* Load the DTB, kernel image, and configuration data from the SD card. */
    if (load_sdcard(&image) != 0) {
        printk(LOG_LEVEL_WARNING, "SMHC: loading failed\n");
    }

    /* Update boot arguments based on configuration file. */
    update_bootargs_from_config(dram_size);

    /* Set up boot parameters for the kernel. */
    if (boot_image_setup((uint8_t *) image.dest, &entry_point)) {
        printk(LOG_LEVEL_ERROR, "boot setup failed\n");
        abort();
    }

    /* Debug message to indicate that the kernel is being booted. */
    printk(LOG_LEVEL_INFO, "booting linux...\n");

    /* Disable MMU, data cache, instruction cache, interrupts, and enable symmetric multiprocessing (SMP) in the kernel. */
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

    /* Debug message to indicate the kernel address that the system is jumping to. */
    printk(LOG_LEVEL_INFO, "jump to kernel address: 0x%x\n\n", image.dest);

    /* Jump to the kernel entry point. */
    kernel_entry = (void (*)(int, int, uint32_t)) entry_point;
    kernel_entry(0, ~0, (uint32_t) image.of_dest);

    /* If the kernel boot fails, jump to FEL mode. */
    jmp_to_fel();

    /* Return 0 to indicate successful execution. */
    return 0;
}
