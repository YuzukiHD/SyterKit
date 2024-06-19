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

#include <mmc/sys-sdcard.h>
#include <sys-clk.h>
#include <sys-dram.h>
#include <sys-i2c.h>
#include <sys-rtc.h>
#include <sys-sid.h>
#include <sys-spi.h>

#include <pmu/axp.h>

#include <fdt_wrapper.h>
#include <ff.h>
#include <libfdt.h>
#include <sys-sdhci.h>
#include <uart.h>

#include "spi_lcd.c"

#define CONFIG_SPLASH_LOAD_ADDR (0x40080000)
#define CONFIG_SPLASH_FILENAME "splash.bin"

#define CONFIG_BL31_FILENAME "bl31.bin"
#define CONFIG_BL31_LOAD_ADDR (0x48000000)

#define CONFIG_DTB_LOAD_ADDR (0x40400000)
#define CONFIG_DTBO_LOAD_ADDR (0x50400000)
#define CONFIG_INITRD_LOAD_ADDR (0x43000000)
#define CONFIG_KERNEL_LOAD_ADDR (0x40800000)

#define CONFIG_SCP_FILENAME "scp.bin"
#define CONFIG_SCP_LOAD_ADDR (0x48100000)

#define CONFIG_EXTLINUX_FILENAME "extlinux/extlinux.conf"
#define CONFIG_EXTLINUX_LOAD_ADDR (0x40020000)

#define CONFIG_PLATFORM_MAGIC "\0RAW\xbe\xe9\0\0"

#define CONFIG_SDMMC_SPEED_TEST_SIZE 1024// (unit: 512B sectors)

#define CONFIG_HEAP_BASE (0x50800000)
#define CONFIG_HEAP_SIZE (16 * 1024 * 1024)

extern sunxi_serial_t uart_dbg;

extern sunxi_i2c_t i2c_pmu;

extern sunxi_sdhci_t sdhci0;
extern sunxi_sdhci_t sdhci2;

extern uint32_t dram_para[32];

extern char *dram_para_name[32];

extern void enable_sram_a3();
extern void rtc_set_vccio_det_spare();
extern void set_rpio_power_mode(void);
extern void sunxi_nsi_init();
extern void gicr_set_waker(void);

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

typedef struct ext_linux_data {
    char *os;
    char *kernel;
    char *initrd;
    char *fdt;
    char *dtbo;
    char *append;
} ext_linux_data_t;

#define FILENAME_MAX_LEN 25
typedef struct {
    uint8_t *bl31_dest;
    char bl31_filename[FILENAME_MAX_LEN];

    uint8_t *scp_dest;
    char scp_filename[FILENAME_MAX_LEN];

    uint8_t *splash_dest;
    char splash_filename[FILENAME_MAX_LEN];

    uint8_t *kernel_dest;
    uint8_t *ramdisk_dest;
    uint8_t *of_dest;
    uint8_t *of_overlay_dest;

    uint8_t *extlinux_dest;
    char extlinux_filename[FILENAME_MAX_LEN];
} image_info_t;

#define IH_COMP_NONE 0      /*  No	 Compression Used	*/
#define IH_COMP_GZIP 1      /* gzip	 Compression Used	*/
#define IH_COMP_BZIP2 2     /* bzip2 Compression Used	*/
#define IH_MAGIC 0x56190527 /* mkimage magic for uinitrd */
#define IH_NMLEN 32         /* Image Name Length	*/
typedef struct image_header {
    uint32_t ih_magic;         /* Image Header Magic Number	*/
    uint32_t ih_hcrc;          /* Image Header CRC Checksum	*/
    uint32_t ih_time;          /* Image Creation Timestamp	*/
    uint32_t ih_size;          /* Image Data Size		*/
    uint32_t ih_load;          /* Data	 Load  Address		*/
    uint32_t ih_ep;            /* Entry Point Address		*/
    uint32_t ih_dcrc;          /* Image Data CRC Checksum	*/
    uint8_t ih_os;             /* Operating System		*/
    uint8_t ih_arch;           /* CPU architecture		*/
    uint8_t ih_type;           /* Image Type			*/
    uint8_t ih_comp;           /* Compression Type		*/
    uint8_t ih_name[IH_NMLEN]; /* Image Name		*/
} image_header_t;

image_info_t image;

#define CHUNK_SIZE 0x160000

static int fatfs_loadimage_size(char *filename, BYTE *dest, uint32_t *file_size) {
    FIL file;
    UINT byte_to_read = CHUNK_SIZE;
    UINT byte_read;
    UINT total_read = 0;
    FRESULT fret;
    int ret = 1;
    uint32_t start, time;

    fret = f_open(&file, filename, FA_OPEN_EXISTING | FA_READ);
    if (fret != FR_OK) {
        printk_warning("FATFS: open, filename: [%s]: error %d\n", filename, fret);
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
    *file_size = total_read;

read_fail:
    fret = f_close(&file);

    printk_info("FATFS: read in %ums at %.2fMB/S\n", time,
                (f32) (total_read / time) / 1024.0f);

open_fail:
    return ret;
}

static int fatfs_loadimage(char *filename, BYTE *dest) {
    return fatfs_loadimage_size(filename, dest, NULL);
}

static int load_sdcard(image_info_t *image) {
    FATFS fs;
    FRESULT fret;
    int ret;
    uint32_t start = time_ms();

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

    printk_info("FATFS: read %s addr=%x\n", image->scp_filename, (uint32_t) image->scp_dest);
    ret = fatfs_loadimage(image->scp_filename, image->scp_dest);
    if (ret)
        return ret;

    printk_info("FATFS: read %s addr=%x\n", image->extlinux_filename, (uint32_t) image->extlinux_dest);
    ret = fatfs_loadimage(image->extlinux_filename, image->extlinux_dest);
    if (ret)
        return ret;

    printk_info("FATFS: read %s addr=%x\n", image->splash_filename, (uint32_t) image->splash_dest);
    ret = fatfs_loadimage(image->splash_filename, image->splash_dest);
    if (ret)
        printk_info("FATFS: Splash load fail, Leave Black Screen.\n");
    else
        LCD_Show_Splash(image->splash_dest);

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

static char *skip_spaces(char *str) {
    while (*str == ' ') str++;
    return str;
}

static char *find_substring(char *source, const char *target) {
    char *pos = strstr(source, target);
    if (pos) {
        return pos + strlen(target);
    }
    return NULL;
}

static char *copy_until_newline_or_end(char *source) {
    if (!source) return NULL;

    source = skip_spaces(source);

    char *end = strchr(source, '\n');
    size_t len;
    if (end) {
        len = end - source;
    } else {
        len = strlen(source);
    }
    char *dest = smalloc(len + 1);
    if (!dest) return NULL;

    strncpy(dest, source, len);
    dest[len] = '\0';
    return dest;
}

static void parse_extlinux_data(char *config, ext_linux_data_t *data) {
    char *start;

    start = find_substring(config, "label ");
    data->os = copy_until_newline_or_end(start);

    start = find_substring(config, "kernel ");
    data->kernel = copy_until_newline_or_end(start);

    start = find_substring(config, "initrd ");
    data->initrd = copy_until_newline_or_end(start);

    start = find_substring(config, "fdt ");
    data->fdt = copy_until_newline_or_end(start);

    start = find_substring(config, "fdtoverlay ");
    data->dtbo = copy_until_newline_or_end(start);

    start = find_substring(config, "append ");
    data->append = copy_until_newline_or_end(start);
}

static int fdt_pack_reg(const void *fdt, void *buf, uint64_t address, uint64_t size) {
    int i;
    int address_cells = fdt_address_cells(fdt, 0);
    int size_cells = fdt_size_cells(fdt, 0);
    char *p = buf;

    if (address_cells == 2)
        *(fdt64_t *) p = cpu_to_fdt64(address);
    else
        *(fdt32_t *) p = cpu_to_fdt32(address);
    p += 4 * address_cells;

    if (size_cells == 2)
        *(fdt64_t *) p = cpu_to_fdt64(size);
    else
        *(fdt32_t *) p = cpu_to_fdt32(size);
    p += 4 * size_cells;

    return p - (char *) buf;
}

static int update_pmu_ext_info_dtb(image_info_t *image) {
    int nodeoffset, pmu_ext_type, err, i;
    uint32_t phandle = 0;

    /* get used pmu_ext node */
    nodeoffset = fdt_path_offset(image->of_dest, "reg-axp1530");
    if (nodeoffset < 0) {
        printk_error("FDT: Could not find nodeoffset for used ext pmu:%s\n", "reg-axp1530");
        return -1;
    }
    /* get used pmu_ext phandle */
    phandle = fdt_get_phandle(image->of_dest, nodeoffset);
    if (!phandle) {
        printk_error("FDT: Could not find phandle for used ext pmu:%s\n", "reg-axp1530");
        return -1;
    }
    printk_debug("get ext power phandle %d\n", phandle);

    /* get cpu@4 node */
    nodeoffset = fdt_path_offset(image->of_dest, "cpu-ext");
    if (nodeoffset < 0) {
        printk_error("FDT: cannot get cpu@4 node\n");
        return -1;
    }

    /* Change cpu-supply to ext dcdc*/
    err = fdt_setprop_u32(image->of_dest, nodeoffset, "cpu-supply", phandle);
    if (err < 0) {
        printk_warning("WARNING: fdt_setprop can't set %s from node %s: %s\n", "compatible", "status", fdt_strerror(err));
        return -1;
    }

    return 0;
}

static inline char to_hex_char(uint8_t value) {
    return (value < 10) ? ('0' + value) : ('A' + value - 10);
}

static void chip_sid_to_mac(uint32_t chip_sid[4], uint8_t mac_address[6]) {
    mac_address[3] = chip_sid[0] & 0xFF;
    mac_address[2] = (chip_sid[1] >> 8) & 0xFF;
    mac_address[1] = chip_sid[1] & 0xFF;
    mac_address[0] = (chip_sid[2] >> 8) & 0xFF;
    mac_address[4] = chip_sid[2] & 0xFF;
    mac_address[5] = 0xFF;
}

static char *get_mac_address_from_sid(uint32_t chip_sid[4], char mac_address_str[18]) {
    uint8_t mac_address[6];
    chip_sid_to_mac(chip_sid, mac_address);
    mac_address_str[0] = to_hex_char(mac_address[0] >> 4);
    mac_address_str[1] = to_hex_char(mac_address[0] & 0x0F);
    mac_address_str[2] = ':';
    mac_address_str[3] = to_hex_char(mac_address[1] >> 4);
    mac_address_str[4] = to_hex_char(mac_address[1] & 0x0F);
    mac_address_str[5] = ':';
    mac_address_str[6] = to_hex_char(mac_address[2] >> 4);
    mac_address_str[7] = to_hex_char(mac_address[2] & 0x0F);
    mac_address_str[8] = ':';
    mac_address_str[9] = to_hex_char(mac_address[3] >> 4);
    mac_address_str[10] = to_hex_char(mac_address[3] & 0x0F);
    mac_address_str[11] = ':';
    mac_address_str[12] = to_hex_char(mac_address[4] >> 4);
    mac_address_str[13] = to_hex_char(mac_address[4] & 0x0F);
    mac_address_str[14] = ':';
    mac_address_str[15] = to_hex_char(mac_address[5] >> 4);
    mac_address_str[16] = to_hex_char(mac_address[5] & 0x0F);
    mac_address_str[17] = '\0';
    return mac_address_str;
}

static int load_extlinux(image_info_t *image, uint64_t dram_size) {
    FATFS fs;
    FRESULT fret;
    ext_linux_data_t data = {0};
    int ret, err = -1;
    uint32_t start;

    uint32_t test_time;

    parse_extlinux_data(image->extlinux_dest, &data);

    printk_debug("os: %s\n", data.os);
    printk_debug("%s: kernel -> %s\n", data.os, data.kernel);
    printk_debug("%s: initrd -> %s\n", data.os, data.initrd);
    printk_debug("%s: fdt -> %s\n", data.os, data.fdt);
    printk_debug("%s: dtbo -> %s\n", data.os, data.dtbo);
    printk_debug("%s: append -> %s\n", data.os, data.append);

    start = time_ms();
    fret = f_mount(&fs, "", 1);
    if (fret != FR_OK) {
        printk_error("FATFS: mount error: %d\n", fret);
        goto _error;
    } else {
        printk_debug("FATFS: mount OK\n");
    }

    printk_info("FATFS: read %s addr=%x\n", data.kernel, (uint32_t) image->kernel_dest);
    ret = fatfs_loadimage(data.kernel, image->kernel_dest);
    if (ret)
        goto _error;

    printk_info("FATFS: read %s addr=%x\n", data.fdt, (uint32_t) image->of_dest);
    ret = fatfs_loadimage(data.fdt, image->of_dest);
    if (ret)
        goto _error;

    /* Check and load ramdisk */
    uint32_t ramdisk_size = 0;
    if (data.initrd != NULL) {
        printk_info("FATFS: read %s addr=%x\n", data.initrd, (uint32_t) image->ramdisk_dest);
        ret = fatfs_loadimage_size(data.initrd, image->ramdisk_dest, &ramdisk_size);
        if (ret) {
            printk_warning("Initrd not find, ramdisk not load.\n");
            ramdisk_size = 0;
        } else {
            printk_info("Initrd load 0x%08x, Size 0x%08x\n", image->ramdisk_dest, ramdisk_size);
        }
    }

    /* umount fs */
    fret = f_mount(0, "", 0);
    if (fret != FR_OK) {
        printk_error("FATFS: unmount error %d\n", fret);
        goto _error;
    } else {
        printk_debug("FATFS: unmount OK\n");
    }
    printk_debug("FATFS: done in %ums\n", time_ms() - start);

    /* Force image.of_dest to be a pointer to fdt_header structure */
    struct fdt_header *dtb_header = (struct fdt_header *) image->of_dest;

    /* Check if DTB header is valid */
    if ((ret = fdt_check_header(dtb_header)) != 0) {
        printk_error("Invalid device tree blob: %s\n", fdt_strerror(ret));
        goto _error;
    }

    /* Get the total size of DTB */
    uint32_t size = fdt_totalsize(image->of_dest);

    printk_debug("FDT dtb size = %d\n", size);

    if ((ret = fdt_increase_size(image->of_dest, 512)) != 0) {
        printk_error("FDT: device tree increase error: %s\n", fdt_strerror(ret));
        goto _error;
    }

    update_pmu_ext_info_dtb(image);

    printk_debug("FDT dtb size = %d\n", fdt_totalsize(image->of_dest));

    int memory_node = fdt_find_or_add_subnode(image->of_dest, 0, "memory");

    if ((ret = fdt_setprop_string(image->of_dest, memory_node, "device_type", "memory")) != 0) {
        printk_error("Can't change memory size node: %s\n", fdt_strerror(ret));
        goto _error;
    }

    uint8_t *tmp_buf = (uint8_t *) smalloc(16 * sizeof(uint8_t));

    /* fix up memory region */
    int len = fdt_pack_reg(image->of_dest, tmp_buf, SDRAM_BASE, (dram_size * 1024 * 1024));

    if ((ret = fdt_setprop(image->of_dest, memory_node, "reg", tmp_buf, len)) != 0) {
        printk_error("Can't change memory base node: %s\n", fdt_strerror(ret));
        goto _error;
    }

    /* Get the offset of "/chosen" node */
    int chosen_node = fdt_find_or_add_subnode(image->of_dest, 0, "chosen");

    uint32_t ramdisk_start = (uint32_t) (uintptr_t) image->ramdisk_dest;
    uint32_t ramdisk_end = (uint32_t) ramdisk_start + (uint32_t) ramdisk_size;
    if (ramdisk_size > 0) {
        uint64_t addr, size;

        /* Check if using uinitrd */
        image_header_t *ramdisk_header = (image_header_t *) image->ramdisk_dest;

        if (ramdisk_header->ih_magic == IH_MAGIC) {
            ramdisk_start += 0x40;
        }

        printk_debug("initrd_start = 0x%08x, initrd_end = 0x%08x\n", ramdisk_start, ramdisk_end);

        int total = fdt_num_mem_rsv(image->of_dest);

        printk_debug("Look for an existing entry %d\n", total);

        /* Look for an existing entry and update it.  If we don't find the entry, we will add a available slot. */
        for (int j = 0; j < total; j++) {
            ret = fdt_get_mem_rsv(image->of_dest, j, &addr, &size);
            if (addr == ramdisk_start) {
                fdt_del_mem_rsv(image->of_dest, j);
                break;
            }
        }

        ret = fdt_add_mem_rsv(image->of_dest, ramdisk_start, ramdisk_end - ramdisk_start);
        if (ret < 0) {
            printk_debug("fdt_initrd: %s\n", fdt_strerror(ret));
            goto _error;
        }

        ret = fdt_setprop_u64(image->of_dest, chosen_node, "linux,initrd-start", (uint64_t) ramdisk_start);
        if (ret < 0) {
            printk_debug("WARNING: could not set linux,initrd-start %s.\n", fdt_strerror(ret));
            goto _error;
        }

        ret = fdt_setprop_u64(image->of_dest, chosen_node, "linux,initrd-end", (uint64_t) ramdisk_end);
        if (ret < 0) {
            printk_debug("WARNING: could not set linux,initrd-end %s.\n", fdt_strerror(ret));
            goto _error;
        }
    }
    len = 0;
    /* Get bootargs string */
    char *bootargs = (char *) smalloc(4096);
    memset(bootargs, 0, 4096);
    char *bootargs_str = (void *) fdt_getprop(image->of_dest, chosen_node, "bootargs", &len);
    if (bootargs_str == NULL) {
        printk_warning("FDT: bootargs is null, using extlinux.conf append.\n");
    } else {
        strcat(bootargs, " ");
        strcat(bootargs, bootargs_str);
    }

    /* Append bootargs */
    strcat(bootargs, data.append);

    int dram_node = fdt_find_or_add_subnode(image->of_dest, 0, "dram");
    /* Kernel only need 0: DRAM_CLK, 24: DRAM_DIV */
    fdt_setprop_u32(image->of_dest, dram_node, dram_para_name[0], dram_para[0]);
    fdt_setprop_u32(image->of_dest, dram_node, dram_para_name[1], dram_para[24]);

    /* Append bootargs mac address */
    uint32_t chip_sid[4];
    chip_sid[0] = read32(SUNXI_SID_SRAM_BASE + 0x0);
    chip_sid[1] = read32(SUNXI_SID_SRAM_BASE + 0x4);
    chip_sid[2] = read32(SUNXI_SID_SRAM_BASE + 0x8);
    chip_sid[3] = read32(SUNXI_SID_SRAM_BASE + 0xc);

    char mac_address_str[18];
    char *mac0_address = get_mac_address_from_sid(chip_sid, mac_address_str);
    strcat(bootargs, " mac0_addr=");
    strcat(bootargs, mac0_address);
    chip_sid[2]++;
    char *mac1_address = get_mac_address_from_sid(chip_sid, mac_address_str);
    strcat(bootargs, " mac1_addr=");
    strcat(bootargs, mac1_address);

_add_dts_size:
    /* Modify bootargs string */
    ret = fdt_setprop_string(image->of_dest, chosen_node, "bootargs", skip_spaces(bootargs));
    if (ret == -FDT_ERR_NOSPACE) {
        printk_debug("FDT: FDT_ERR_NOSPACE, Size = %d, Increase Size = %d\n", size, 512);
        ret = fdt_increase_size(image->of_dest, 512);
        if (!ret) {
            goto _add_dts_size;
        } else {
            printk_error("DTB: Can't increase blob size: %s\n", fdt_strerror(ret));
            goto _error;
        }
    } else if (ret < 0) {
        printk_error("Can't change bootargs node: %s\n", fdt_strerror(ret));
        goto _error;
    }

    /* Get the total size of DTB */
    printk_debug("Modify FDT Size = %d\n", fdt_totalsize(image->of_dest));

    if (ret < 0) {
        printk_error("libfdt fdt_setprop() error: %s\n", fdt_strerror(ret));
        goto _error;
    }

    /* Check and load dtbo */
    if (data.dtbo != NULL) {
        printk_info("FATFS: read %s addr=%x\n", data.dtbo, (uint32_t) image->of_overlay_dest);
        ret = fatfs_loadimage(data.dtbo, image->of_overlay_dest);
        if (ret) {
            printk_warning("dtb overlay not find, overlay not applied.\n");
            goto _error;
        } else {
            printk_info("dtbo load 0x%08x\n", image->of_overlay_dest);
        }

        if (!fdt_check_header(image->of_overlay_dest)) {
            printk_warning("dtb overlay not valid, error = %s, overlay not applyed.\n", fdt_strerror(err));
            goto _error;
        } else {
            ret = fdt_overlay_apply_verbose(image->of_dest, image->of_overlay_dest);
            if (ret) {
                printk_warning("dtb overlay not success applied, overlay not applied.\n");
            }
        }
    }

    err = 0;
_error:
    return err;
}

static int abortboot_single_key(int bootdelay) {
    int abort = 0;
    unsigned long ts;

    printk_info("Hit any key to stop autoboot: %2d ", bootdelay);

    /* Check if key already pressed */
    if (tstc()) {       /* we got a key press */
        uart_getchar(); /* consume input */
        printk(LOG_LEVEL_MUTE, "\b\b\b%2d", bootdelay);
        abort = 0; /* auto boot */
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

int main(void) {
    sunxi_serial_init(&uart_dbg);

    arm32_dcache_enable();
    arm32_icache_enable();

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

    pmu_axp2202_set_vol(&i2c_pmu, "bldo3", 1800, 1);
    pmu_axp2202_set_vol(&i2c_pmu, "bldo1", 1800, 1);
    pmu_axp2202_set_vol(&i2c_pmu, "cldo1", 2100, 1);

    pmu_axp2202_dump(&i2c_pmu);
    pmu_axp1530_dump(&i2c_pmu);

    sunxi_clk_set_cpu_pll(1416);

    enable_sram_a3();

    /* Initialize the DRAM and enable memory management unit (MMU). */
    uint64_t dram_size = sunxi_dram_init((void *) dram_para);

    printk_debug("DRAM Size = %dM\n", dram_size);

    sunxi_clk_dump();

    arm32_mmu_enable(SDRAM_BASE, dram_size);

    /* Initialize the small memory allocator. */
    smalloc_init(CONFIG_HEAP_BASE, CONFIG_HEAP_SIZE);

    LCD_Init();

    sunxi_nsi_init();

    /* Clear the image_info_t struct. */
    memset(&image, 0, sizeof(image_info_t));

    image.bl31_dest = (uint8_t *) CONFIG_BL31_LOAD_ADDR;
    image.scp_dest = (uint8_t *) CONFIG_SCP_LOAD_ADDR;
    image.extlinux_dest = (uint8_t *) CONFIG_EXTLINUX_LOAD_ADDR;
    image.of_dest = (uint8_t *) CONFIG_DTB_LOAD_ADDR;
    image.ramdisk_dest = (uint8_t *) CONFIG_INITRD_LOAD_ADDR;
    image.kernel_dest = (uint8_t *) CONFIG_KERNEL_LOAD_ADDR;
    image.splash_dest = (uint8_t *) CONFIG_SPLASH_LOAD_ADDR;
    image.of_overlay_dest = (uint8_t *) CONFIG_DTBO_LOAD_ADDR;

    strcpy(image.bl31_filename, CONFIG_BL31_FILENAME);
    strcpy(image.scp_filename, CONFIG_SCP_FILENAME);
    strcpy(image.extlinux_filename, CONFIG_EXTLINUX_FILENAME);
    strcpy(image.splash_filename, CONFIG_SPLASH_FILENAME);

    /* Initialize the SD host controller. */
    if (sunxi_sdhci_init(&sdhci0) != 0) {
        printk_error("SMHC: %s controller init failed\n", sdhci0.name);
        LCD_ShowString(0, 92, "SMHC: SDC0 controller init failed", SPI_LCD_COLOR_GREEN, SPI_LCD_COLOR_BLACK, 12);
        goto _fail;
    } else {
        printk_info("SMHC: %s controller initialized\n", sdhci0.name);
    }

    /* Initialize the SD card and check if initialization is successful. */
    if (sdmmc_init(&card0, &sdhci0) != 0) {
        printk_warning("SMHC: SDC0 init failed, init SDC2...\n");
        /* Initialize the SD host controller. */
        if (sunxi_sdhci_init(&sdhci2) != 0) {
            printk_error("SMHC: %s controller init failed\n", sdhci2.name);
            LCD_ShowString(0, 92, "SMHC: SDC2 controller init failed", SPI_LCD_COLOR_GREEN, SPI_LCD_COLOR_BLACK, 12);
            goto _fail;
        } else {
            printk_info("SMHC: %s controller initialized\n", sdhci2.name);
        }

        if (sdmmc_init(&card0, &sdhci2) != 0) {
            printk_warning("SMHC: SDC2 init failed.\n");
            goto _fail;
        }
    }

    /* Load the DTB, kernel image, and configuration data from the SD card. */
    if (load_sdcard(&image) != 0) {
        printk_warning("SMHC: loading failed, try to boot from SDC2\n");

        if (sunxi_sdhci_init(&sdhci2) != 0) {
            printk_error("SMHC: %s controller init failed\n", sdhci2.name);
            LCD_ShowString(0, 92, "SMHC: SDC2 controller init failed", SPI_LCD_COLOR_GREEN, SPI_LCD_COLOR_BLACK, 12);
            goto _fail;
        } else {
            if (sdmmc_init(&card0, &sdhci2) != 0) {
                printk_warning("SMHC: SDC2 init failed.\n");
                goto _fail;
            } else {
                printk_info("SMHC: %s controller initialized\n", sdhci2.name);
                if (load_sdcard(&image) != 0) {
                    printk_error("SMHC: loading boot info failed, check your boot media.\n");
                    goto _fail;
                }
            }
        }
    }

    LCD_Open_BLK();

    if (load_extlinux(&image, dram_size) != 0) {
        printk_error("EXTLINUX: load extlinux failed\n");
        LCD_ShowString(0, 92, "EXTLINUX: load extlinux failed", SPI_LCD_COLOR_GREEN, SPI_LCD_COLOR_BLACK, 12);
        goto _fail;
    }

    printk_info("EXTLINUX: load extlinux done, now booting...\n");

    atf_head_t *atf_head = (atf_head_t *) image.bl31_dest;

    atf_head->dtb_base = (uint32_t) image.of_dest;
    atf_head->nos_base = (uint32_t) image.kernel_dest;

    /* Fill platform magic */
    memcpy(atf_head->platform, CONFIG_PLATFORM_MAGIC, 8);

    printk_info("ATF: Kernel addr: 0x%08x\n", atf_head->nos_base);
    printk_info("ATF: Kernel DTB addr: 0x%08x\n", atf_head->dtb_base);

    /* flush buffer */
    LCD_ShowString(0, 0, "SyterKit Now Booting Linux", SPI_LCD_COLOR_GREEN, SPI_LCD_COLOR_BLACK, 12);
    LCD_ShowString(0, 12, "Kernel Addr: 0x40800000", SPI_LCD_COLOR_GREEN, SPI_LCD_COLOR_BLACK, 12);
    LCD_ShowString(0, 24, "DTB Addr: 0x40400000", SPI_LCD_COLOR_GREEN, SPI_LCD_COLOR_BLACK, 12);

    clean_syterkit_data();

    gicr_set_waker();

    jmp_to_arm64(CONFIG_BL31_LOAD_ADDR);

    printk_info("Back to SyterKit\n");

    // if kernel boot not success, jump to fel.
    jmp_to_fel();

_fail:
    LCD_ShowString(0, 0, "SyterKit Boot Failed", SPI_LCD_COLOR_RED, SPI_LCD_COLOR_BLACK, 12);
    LCD_ShowString(0, 12, "Please Connect UART for Debug info", SPI_LCD_COLOR_RED, SPI_LCD_COLOR_BLACK, 12);
    LCD_ShowString(0, 24, "Error Info:", SPI_LCD_COLOR_RED, SPI_LCD_COLOR_BLACK, 12);
    LCD_Open_BLK();

    printk_error("SyterKit Boot Failed, System into software_interrupt panic. Please reset your board.\n");

    panic();

    return 0;
}