/* SPDX-License-Identifier: Apache-2.0 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>
#include <ctype.h>

#include <config.h>
#include <log.h>

#include <arm32.h>
#include <common.h>
#include <jmp.h>

#include "sys-dram.h"
#include "sys-sdcard.h"
#include "sys-sid.h"
#include "sys-spi.h"

#include "ff.h"
#include "libfdt.h"

#define CONFIG_DTB_FILENAME "sunxi.dtb"
#define CONFIG_DTB_LOADADDR (0x41008000)

#define MAX_LEVEL 32    /* how deeply nested we will go */
#define SCRATCHPAD 1024 /* bytes of scratchpad memory */
#define CMD_FDT_MAX_DUMP 64

#define CONFIG_SDMMC_SPEED_TEST_SIZE 1024// (unit: 512B sectors)

sunxi_uart_t uart_dbg = {
        .base = 0x02500000,
        .id = 0,
        .gpio_tx = {GPIO_PIN(PORTH, 9), GPIO_PERIPH_MUX5},
        .gpio_rx = {GPIO_PIN(PORTH, 10), GPIO_PERIPH_MUX5},
};

sunxi_uart_t uart_e907 = {
        .base = 0x02500C00,
        .id = 3,
        .gpio_tx = {GPIO_PIN(PORTE, 0), GPIO_PERIPH_MUX7},
        .gpio_rx = {GPIO_PIN(PORTE, 1), GPIO_PERIPH_MUX7},
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

#define FILENAME_MAX_LEN 64
typedef struct {
    unsigned int offset;
    unsigned int length;
    unsigned char *dest;

    char filename[FILENAME_MAX_LEN];
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
        printk(LOG_LEVEL_ERROR, "FATFS: open, filename: [%s]: error %d\r\n", filename, fret);
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
        printk(LOG_LEVEL_ERROR, "FATFS: read: error %d\r\n", fret);
        ret = -1;
        goto read_fail;
    }
    ret = 0;

read_fail:
    fret = f_close(&file);

    printk(LOG_LEVEL_DEBUG, "FATFS: read in %ums at %.2fMB/S\r\n", time,
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
    printk(LOG_LEVEL_DEBUG, "SDMMC: speedtest %uKB in %ums at %uKB/S\r\n",
           (CONFIG_SDMMC_SPEED_TEST_SIZE * 512) / 1024, test_time,
           (CONFIG_SDMMC_SPEED_TEST_SIZE * 512) / test_time);

    start = time_ms();

    fret = f_mount(&fs, "", 1);
    if (fret != FR_OK) {
        printk(LOG_LEVEL_ERROR, "FATFS: mount error: %d\r\n", fret);
        return -1;
    } else {
        printk(LOG_LEVEL_DEBUG, "FATFS: mount OK\r\n");
    }

    printk(LOG_LEVEL_INFO, "FATFS: read %s addr=%x\r\n", image->filename,
           (unsigned int) image->dest);
    ret = fatfs_loadimage(image->filename, image->dest);
    if (ret)
        return ret;

    /* umount fs */
    fret = f_mount(0, "", 0);
    if (fret != FR_OK) {
        printk(LOG_LEVEL_ERROR, "FATFS: unmount error %d\r\n", fret);
        return -1;
    } else {
        printk(LOG_LEVEL_DEBUG, "FATFS: unmount OK\r\n");
    }
    printk(LOG_LEVEL_DEBUG, "FATFS: done in %ums\r\n", time_ms() - start);

    return 0;
}

void show_banner(void) {
    uint32_t id[4];

    printk(LOG_LEVEL_MUTE, "\r\n");
    printk(LOG_LEVEL_INFO, " _____     _           _____ _ _   \r\n");
    printk(LOG_LEVEL_INFO, "|   __|_ _| |_ ___ ___|  |  |_| |_ \r\n");
    printk(LOG_LEVEL_INFO, "|__   | | |  _| -_|  _|    -| | _| \r\n");
    printk(LOG_LEVEL_INFO, "|_____|_  |_| |___|_| |__|__|_|_|  \r\n");
    printk(LOG_LEVEL_INFO, "      |___|                        \r\n");
    printk(LOG_LEVEL_INFO, "***********************************\r\n");
    printk(LOG_LEVEL_INFO, " %s V0.1.1 Commit: %s\r\n", PROJECT_NAME,
           PROJECT_GIT_HASH);
    printk(LOG_LEVEL_INFO, "***********************************\r\n");

    id[0] = read32(0x03006200 + 0x0);
    id[1] = read32(0x03006200 + 0x4);
    id[2] = read32(0x03006200 + 0x8);
    id[3] = read32(0x03006200 + 0xc);

    printk(LOG_LEVEL_INFO, "Chip ID is: %08x%08x%08x%08x\r\n", id[0], id[1],
           id[2], id[3]);
}

static int is_printable_string(const void *data, int len) {
    const char *s = data;

    /* zero length is not */
    if (len == 0)
        return 0;

    /* must terminate with zero or '\n' */
    if (s[len - 1] != '\0' && s[len - 1] != '\n')
        return 0;

    /* printable or a null byte (concatenated strings) */
    while (((*s == '\0') || isprint(*s) || isspace(*s)) && (len > 0)) {
        /*
		 * If we see a null, there are three possibilities:
		 * 1) If len == 1, it is the end of the string, printable
		 * 2) Next character also a null, not printable.
		 * 3) Next character not a null, continue to check.
		 */
        if (s[0] == '\0') {
            if (len == 1)
                return 1;
            if (s[1] == '\0')
                return 0;
        }
        s++;
        len--;
    }

    /* Not the null termination, or not done yet: not printable */
    if (*s != '\0' || (len != 0))
        return 0;

    return 1;
}

static void print_data(const void *data, int len) {
    int j;

    /* no data, don't print */
    if (len == 0)
        return;

    /*
	 * It is a string, but it may have multiple strings (embedded '\0's).
	 */
    if (is_printable_string(data, len)) {
        printk(LOG_LEVEL_MUTE, "\"");
        j = 0;
        while (j < len) {
            if (j > 0)
                printk(LOG_LEVEL_MUTE, "\", \"");
            printk(LOG_LEVEL_MUTE, data);
            j += strlen(data) + 1;
            data += strlen(data) + 1;
        }
        printk(LOG_LEVEL_MUTE, "\"");
        return;
    }

    if ((len % 4) == 0) {
        if (len > CMD_FDT_MAX_DUMP)
            printk(LOG_LEVEL_MUTE, "* 0x%p [0x%08x]", data, len);
        else {
            const uint32_t *p;

            printk(LOG_LEVEL_MUTE, "<");
            for (j = 0, p = data; j < len / 4; j++)
                printk(LOG_LEVEL_MUTE, "0x%08x%s", fdt32_to_cpu(p[j]),
                       j < (len / 4 - 1) ? " " : "");
            printk(LOG_LEVEL_MUTE, ">");
        }
    } else { /* anything else... hexdump */
        if (len > CMD_FDT_MAX_DUMP)
            printk(LOG_LEVEL_MUTE, "* 0x%p [0x%08x]", data, len);
        else {
            const u8 *s;

            printk(LOG_LEVEL_MUTE, "[");
            for (j = 0, s = data; j < len; j++)
                printk(LOG_LEVEL_MUTE, "%02x%s", s[j], j < len - 1 ? " " : "");
            printk(LOG_LEVEL_MUTE, "]");
        }
    }
}

int fdt_print(unsigned char *working_fdt, const char *pathp, char *prop, int depth) {
    static char tabs[MAX_LEVEL + 1] =
            "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t"
            "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";
    const void *nodep; /* property node pointer */
    int nodeoffset;    /* node offset from libfdt */
    int nextoffset;    /* next node offset from libfdt */
    uint32_t tag;      /* tag */
    int len;           /* length of the property */
    int level = 0;     /* keep track of nesting level */
    const struct fdt_property *fdt_prop;

    nodeoffset = fdt_path_offset(working_fdt, pathp);
    if (nodeoffset < 0) {
        /*
		 * Not found or something else bad happened.
		 */
        printk(LOG_LEVEL_MUTE, "libfdt fdt_path_offset() returned %s\r\n",
               fdt_strerror(nodeoffset));
        return 1;
    }
    /*
	 * The user passed in a property as well as node path.
	 * Print only the given property and then return.
	 */
    if (prop) {
        nodep = fdt_getprop(working_fdt, nodeoffset, prop, &len);
        if (len == 0) {
            /* no property value */
            printk(LOG_LEVEL_MUTE, "%s %s\r\n", pathp, prop);
            return 0;
        } else if (nodep && len > 0) {
            printk(LOG_LEVEL_MUTE, "%s = ", prop);
            print_data(nodep, len);
            printk(LOG_LEVEL_MUTE, "\r\n");
            return 0;
        } else {
            printk(LOG_LEVEL_MUTE, "libfdt fdt_getprop(): %s\r\n", fdt_strerror(len));
            return 1;
        }
    }

    /*
	 * The user passed in a node path and no property,
	 * print the node and all subnodes.
	 */
    while (level >= 0) {
        tag = fdt_next_tag(working_fdt, nodeoffset, &nextoffset);
        switch (tag) {
            case FDT_BEGIN_NODE:
                pathp = fdt_get_name(working_fdt, nodeoffset, NULL);
                if (level <= depth) {
                    if (pathp == NULL)
                        pathp = "/* NULL pointer error */";
                    if (*pathp == '\0')
                        pathp = "/"; /* root is nameless */
                    printk(LOG_LEVEL_MUTE, "%s%s {\r\n",
                           &tabs[MAX_LEVEL - level], pathp);
                }
                level++;
                if (level >= MAX_LEVEL) {
                    printk(LOG_LEVEL_MUTE, "Nested too deep, aborting.\r\n");
                    return 1;
                }
                break;
            case FDT_END_NODE:
                level--;
                if (level <= depth)
                    printk(LOG_LEVEL_MUTE, "%s};\r\n", &tabs[MAX_LEVEL - level]);
                if (level == 0) {
                    level = -1; /* exit the loop */
                }
                break;
            case FDT_PROP:
                fdt_prop = fdt_offset_ptr(working_fdt, nodeoffset, sizeof(*fdt_prop));
                pathp = fdt_string(working_fdt, fdt32_to_cpu(fdt_prop->nameoff));
                len = fdt32_to_cpu(fdt_prop->len);
                nodep = fdt_prop->data;
                if (len < 0) {
                    printk(LOG_LEVEL_MUTE, "libfdt fdt_getprop(): %s\r\n", fdt_strerror(len));
                    return 1;
                } else if (len == 0) {
                    /* the property has no value */
                    if (level <= depth)
                        printk(LOG_LEVEL_MUTE, "%s%s;\r\n", &tabs[MAX_LEVEL - level], pathp);
                } else {
                    if (level <= depth) {
                        printk(LOG_LEVEL_MUTE, "%s%s = ", &tabs[MAX_LEVEL - level], pathp);
                        print_data(nodep, len);
                        printk(LOG_LEVEL_MUTE, ";\r\n");
                    }
                }
                break;
            case FDT_NOP:
                printk(LOG_LEVEL_MUTE, "%s/* NOP */\r\n", &tabs[MAX_LEVEL - level]);
                break;
            case FDT_END:
                return 1;
            default:
                if (level <= depth)
                    printk(LOG_LEVEL_MUTE, "Unknown tag 0x%08X\r\n", tag);
                return 1;
        }
        nodeoffset = nextoffset;
    }
    return 0;
}

int main(void) {
    sunxi_uart_init(&uart_dbg);

    show_banner();

    sunxi_clk_init();

    sunxi_dram_init();

    sunxi_clk_dump();

    memset(&image, 0, sizeof(image_info_t));

    image.dest = (uint8_t *) CONFIG_DTB_LOADADDR;

    strcpy(image.filename, CONFIG_DTB_FILENAME);

    if (sunxi_sdhci_init(&sdhci0) != 0) {
        printk(LOG_LEVEL_ERROR, "SMHC: %s controller init failed\r\n",
               sdhci0.name);
        return 0;
    } else {
        printk(LOG_LEVEL_INFO,
               "SMHC: %s controller v%x initialized\r\n", sdhci0.name,
               sdhci0.reg->vers);
    }

    if (sdmmc_init(&card0, &sdhci0) != 0) {
        printk(LOG_LEVEL_ERROR, "SMHC: init failed\r\n");
        return 0;
    }

    if (load_sdcard(&image) != 0) {
        printk(LOG_LEVEL_ERROR, "SMHC: loading failed\r\n");
        return 0;
    }

    struct fdt_header *dtb_header = (struct fdt_header *) image.dest;

    int err = 0;

    if ((err = fdt_check_header(dtb_header)) != 0) {
        printk(LOG_LEVEL_MUTE, "Invalid device tree blob: %s\n", fdt_strerror(err));
        return -1;
    }

    uint32_t size = fdt_totalsize(image.dest);

    printk(LOG_LEVEL_INFO, "DTB FDT Size = 0x%x\r\n", size);

    fdt_print(image.dest, "/", NULL, MAX_LEVEL);

    int len = 0;
    uint32_t bootargs_node = fdt_path_offset(image.dest, "/chosen");
    char *bootargs_str = (void *) fdt_getprop(image.dest, bootargs_node, "bootargs", &len);

    printk(LOG_LEVEL_INFO, "DTB OLD bootargs = \"%s\"\r\n", bootargs_str);

    char *new_bootargs_str = "earlyprintk=sunxi-uart,0x02500C00 root=/dev/mmcblk0p3 rootwait loglevel=8 initcall_debug=0 console=ttyS0 init=/init";

    printk(LOG_LEVEL_INFO, "Now set bootargs to \"%s\"\r\n", new_bootargs_str);

    err = fdt_setprop(image.dest, bootargs_node, "bootargs", new_bootargs_str, strlen(new_bootargs_str) + 1);

    if (err < 0) {
        printk(LOG_LEVEL_ERROR, "libfdt fdt_setprop() error: %s\r\n", fdt_strerror(err));
        abort();
    }

    char *updated_bootargs_str = (void *) fdt_getprop(image.dest, bootargs_node, "bootargs", &len);

    printk(LOG_LEVEL_INFO, "DTB NEW bootargs = \"%s\"\r\n", updated_bootargs_str);

    abort();

    jmp_to_fel();

    return 0;
}