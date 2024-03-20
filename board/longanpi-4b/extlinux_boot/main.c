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
#include <ff.h>
#include <libfdt.h>
#include <sys-sdhci.h>
#include <uart.h>

#define CONFIG_BL31_FILENAME "bl31.bin"
#define CONFIG_BL31_LOAD_ADDR (0x48000000)

#define CONFIG_DTB_LOAD_ADDR (0x40400000)
#define CONFIG_INITRD_LOAD_ADDR (0x43000000)
#define CONFIG_KERNEL_LOAD_ADDR (0x40800000)

#define CONFIG_SCP_FILENAME "scp.bin"
#define CONFIG_SCP_LOAD_ADDR (0x48100000)

#define CONFIG_EXTLINUX_FILENAME "extlinux/extlinux.conf"
#define CONFIG_EXTLINUX_LOAD_ADDR (0x40020000)

#define CONFIG_PLATFORM_MAGIC "\0RAW\xbe\xe9\0\0"

#define CONFIG_SDMMC_SPEED_TEST_SIZE 1024// (unit: 512B sectors)

#define CONFIG_DEFAULT_BOOTDELAY 3

#define CONFIG_HEAP_BASE (0x50800000)
#define CONFIG_HEAP_SIZE (16 * 1024 * 1024)

extern sunxi_serial_t uart_dbg;

extern sunxi_i2c_t i2c_pmu;

extern sdhci_t sdhci0;

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

typedef struct ext_linux_data {
    char *os;
    char *kernel;
    char *initrd;
    char *fdt;
    char *append;
} ext_linux_data_t;

#define FILENAME_MAX_LEN 25
typedef struct {
    uint8_t *bl31_dest;
    char bl31_filename[FILENAME_MAX_LEN];

    uint8_t *scp_dest;
    char scp_filename[FILENAME_MAX_LEN];

    uint8_t *kernel_dest;
    uint8_t *ramdisk_dest;
    uint8_t *of_dest;

    uint8_t *extlinux_dest;
    char extlinux_filename[FILENAME_MAX_LEN];
} image_info_t;

image_info_t image;

#define CHUNK_SIZE 0x20000

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
    *file_size = total_read;

read_fail:
    fret = f_close(&file);

    printk(LOG_LEVEL_INFO, "FATFS: read in %ums at %.2fMB/S\n", time,
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

    printk(LOG_LEVEL_INFO, "FATFS: read %s addr=%x\n", image->bl31_filename, (uint32_t) image->bl31_dest);
    ret = fatfs_loadimage(image->bl31_filename, image->bl31_dest);
    if (ret)
        return ret;

    printk(LOG_LEVEL_INFO, "FATFS: read %s addr=%x\n", image->scp_filename, (uint32_t) image->scp_dest);
    ret = fatfs_loadimage(image->scp_filename, image->scp_dest);
    if (ret)
        return ret;

    printk(LOG_LEVEL_INFO, "FATFS: read %s addr=%x\n", image->extlinux_filename, (uint32_t) image->extlinux_dest);
    ret = fatfs_loadimage(image->extlinux_filename, image->extlinux_dest);
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
        printk(LOG_LEVEL_ERROR, "FDT: Could not find nodeoffset for used ext pmu:%s\n", "reg-axp1530");
        return -1;
    }
    /* get used pmu_ext phandle */
    phandle = fdt_get_phandle(image->of_dest, nodeoffset);
    if (!phandle) {
        printk(LOG_LEVEL_ERROR, "FDT: Could not find phandle for used ext pmu:%s\n", "reg-axp1530");
        return -1;
    }
    printk(LOG_LEVEL_DEBUG, "get ext power phandle %d\n", phandle);

    /* get cpu@4 node */
    nodeoffset = fdt_path_offset(image->of_dest, "cpu-ext");
    if (nodeoffset < 0) {
        printk(LOG_LEVEL_ERROR, "FDT: cannot get cpu@4 node\n");
        return -1;
    }

    /* Change cpu-supply to ext dcdc*/
    err = fdt_setprop_u32(image->of_dest, nodeoffset, "cpu-supply", phandle);
    if (err < 0) {
        printk(LOG_LEVEL_WARNING, "WARNING: fdt_setprop can't set %s from node %s: %s\n", "compatible", "status", fdt_strerror(err));
        return -1;
    }

    return 0;
}

static int load_extlinux(image_info_t *image, uint64_t dram_size) {
    FATFS fs;
    FRESULT fret;
    ext_linux_data_t data = {0};
    int ret, err = -1;
    uint32_t start;

    uint32_t test_time;

    parse_extlinux_data(image->extlinux_dest, &data);

    printk(LOG_LEVEL_DEBUG, "os: %s\n", data.os);
    printk(LOG_LEVEL_DEBUG, "%s: kernel -> %s\n", data.os, data.kernel);
    printk(LOG_LEVEL_DEBUG, "%s: initrd -> %s\n", data.os, data.initrd);
    printk(LOG_LEVEL_DEBUG, "%s: fdt -> %s\n", data.os, data.fdt);
    printk(LOG_LEVEL_DEBUG, "%s: append -> %s\n", data.os, data.append);

    start = time_ms();
    fret = f_mount(&fs, "", 1);
    if (fret != FR_OK) {
        printk(LOG_LEVEL_ERROR, "FATFS: mount error: %d\n", fret);
        goto _error;
    } else {
        printk(LOG_LEVEL_DEBUG, "FATFS: mount OK\n");
    }

    printk(LOG_LEVEL_INFO, "FATFS: read %s addr=%x\n", data.kernel, (uint32_t) image->kernel_dest);
    ret = fatfs_loadimage(data.kernel, image->kernel_dest);
    if (ret)
        goto _error;

    printk(LOG_LEVEL_INFO, "FATFS: read %s addr=%x\n", data.fdt, (uint32_t) image->of_dest);
    ret = fatfs_loadimage(data.fdt, image->of_dest);
    if (ret)
        goto _error;

    /* Check and load ramdisk */
    uint32_t ramdisk_size = 0;
    if (data.initrd != NULL) {
        printk(LOG_LEVEL_INFO, "FATFS: read %s addr=%x\n", data.initrd, (uint32_t) image->ramdisk_dest);
        ret = fatfs_loadimage_size(data.initrd, image->ramdisk_dest, &ramdisk_size);
        if (ret) {
            printk(LOG_LEVEL_WARNING, "Initrd not find, ramdisk not load.\n");
            ramdisk_size = 0;
        } else {
            printk(LOG_LEVEL_INFO, "Initrd load 0x%08x, Size 0x%08x\n", image->ramdisk_dest, ramdisk_size);
        }
    }

    /* umount fs */
    fret = f_mount(0, "", 0);
    if (fret != FR_OK) {
        printk(LOG_LEVEL_ERROR, "FATFS: unmount error %d\n", fret);
        goto _error;
    } else {
        printk(LOG_LEVEL_DEBUG, "FATFS: unmount OK\n");
    }
    printk(LOG_LEVEL_DEBUG, "FATFS: done in %ums\n", time_ms() - start);

    /* Force image.of_dest to be a pointer to fdt_header structure */
    struct fdt_header *dtb_header = (struct fdt_header *) image->of_dest;

    /* Check if DTB header is valid */
    if ((ret = fdt_check_header(dtb_header)) != 0) {
        printk(LOG_LEVEL_ERROR, "Invalid device tree blob: %s\n", fdt_strerror(ret));
        goto _error;
    }

    /* Get the total size of DTB */
    uint32_t size = fdt_totalsize(image->of_dest);

    printk(LOG_LEVEL_DEBUG, "FDT dtb size = %d\n", size);

    if ((ret = fdt_increase_size(image->of_dest, 512)) != 0) {
        printk(LOG_LEVEL_ERROR, "FDT: device tree increase error: %s\n", fdt_strerror(ret));
        goto _error;
    }

    update_pmu_ext_info_dtb(image);

    printk(LOG_LEVEL_DEBUG, "FDT dtb size = %d\n", fdt_totalsize(image->of_dest));

    int memory_node = fdt_find_or_add_subnode(image->of_dest, 0, "memory");

    if ((ret = fdt_setprop_string(image->of_dest, memory_node, "device_type", "memory")) != 0) {
        printk(LOG_LEVEL_ERROR, "Can't change memory size node: %s\n", fdt_strerror(ret));
        goto _error;
    }

    uint8_t *tmp_buf = (uint8_t *) smalloc(16 * sizeof(uint8_t));

    /* fix up memory region */
    int len = fdt_pack_reg(image->of_dest, tmp_buf, SDRAM_BASE, (dram_size * 1024 * 1024));

    if ((ret = fdt_setprop(image->of_dest, memory_node, "reg", tmp_buf, len)) != 0) {
        printk(LOG_LEVEL_ERROR, "Can't change memory base node: %s\n", fdt_strerror(ret));
        goto _error;
    }
    /* clean tmp_buf */
    sfree(tmp_buf);
    tmp_buf = NULL;

    /* Get the offset of "/chosen" node */
    int chosen_node = fdt_find_or_add_subnode(image->of_dest, 0, "chosen");

    uint64_t ramdisk_start = (uint64_t) (uintptr_t) image->ramdisk_dest;
    uint64_t ramdisk_end = ramdisk_start + ramdisk_size;
    if (ramdisk_size > 0) {
        uint64_t addr, size;

        printk(LOG_LEVEL_DEBUG, "initrd_start = 0x%08x, initrd_end = 0x%08x\n", ramdisk_start, ramdisk_end);
        int total = fdt_num_mem_rsv(image->of_dest);

        printk(LOG_LEVEL_DEBUG, "Look for an existing entry %d\n", total);

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
            printk(LOG_LEVEL_DEBUG, "fdt_initrd: %s\n", fdt_strerror(ret));
            goto _error;
        }

        ret = fdt_setprop_u64(image->of_dest, chosen_node, "linux,initrd-start", (uint64_t) ramdisk_start);
        if (ret < 0) {
            printk(LOG_LEVEL_DEBUG, "WARNING: could not set linux,initrd-start %s.\n", fdt_strerror(ret));
            goto _error;
        }

        ret = fdt_setprop_u64(image->of_dest, chosen_node, "linux,initrd-end", (uint64_t) ramdisk_end);
        if (ret < 0) {
            printk(LOG_LEVEL_DEBUG, "WARNING: could not set linux,initrd-end %s.\n", fdt_strerror(ret));
            goto _error;
        }
    }

    len = 0;
    /* Get bootargs string */
    char *bootargs_str = (void *) fdt_getprop(image->of_dest, chosen_node, "bootargs", &len);
    if (bootargs_str == NULL) {
        printk(LOG_LEVEL_WARNING, "FDT: bootargs is null, using extlinux.conf append.\n");
        bootargs_str = (char *) smalloc(strlen(data.append) + 1);
        bootargs_str[0] = '\0';
    } else {
        size_t str_len = strlen(bootargs_str);
        bootargs_str = (char *) srealloc(bootargs_str, str_len + strlen(data.append) + 2);
        strcat(bootargs_str, " ");
    }

    strcat(bootargs_str, data.append);

    printk(LOG_LEVEL_INFO, "Kernel cmdline = [%s]\n", bootargs_str);

_add_dts_size:
    /* Modify bootargs string */
    ret = fdt_setprop_string(image->of_dest, chosen_node, "bootargs", skip_spaces(bootargs_str));
    if (ret == -FDT_ERR_NOSPACE) {
        printk(LOG_LEVEL_DEBUG, "FDT: FDT_ERR_NOSPACE, Size = %d, Increase Size = %d\n", size, 512);
        ret = fdt_increase_size(image->of_dest, 512);
        if (!ret) {
            goto _add_dts_size;
        } else {
            printk(LOG_LEVEL_ERROR, "DTB: Can't increase blob size: %s\n", fdt_strerror(ret));
            goto _bootargs_error;
        }
    } else if (ret < 0) {
        printk(LOG_LEVEL_ERROR, "Can't change bootargs node: %s\n", fdt_strerror(ret));
        goto _bootargs_error;
    }

    /* Get the total size of DTB */
    printk(LOG_LEVEL_DEBUG, "Modify FDT Size = %d\n", fdt_totalsize(image->of_dest));

    if (ret < 0) {
        printk(LOG_LEVEL_ERROR, "libfdt fdt_setprop() error: %s\n", fdt_strerror(ret));
        goto _bootargs_error;
    }

    err = 0;
_bootargs_error:
    if (bootargs_str != NULL)
        sfree(bootargs_str);

_error:
    sfree(data.os);
    sfree(data.kernel);
    sfree(data.initrd);
    sfree(data.fdt);
    sfree(data.append);
    return err;
}

msh_declare_command(boot);
msh_define_help(boot, "boot to linux", "Usage: boot\n");
int cmd_boot(int argc, const char **argv) {
    atf_head_t *atf_head = (atf_head_t *) image.bl31_dest;

    atf_head->dtb_base = (uint32_t) image.of_dest;
    atf_head->nos_base = (uint32_t) image.kernel_dest;

    /* Fill platform magic */
    memcpy(atf_head->platform, CONFIG_PLATFORM_MAGIC, 8);

    printk(LOG_LEVEL_INFO, "ATF: Kernel addr: 0x%08x\n", atf_head->nos_base);
    printk(LOG_LEVEL_INFO, "ATF: Kernel DTB addr: 0x%08x\n", atf_head->dtb_base);

    clean_syterkit_data();

    jmp_to_arm64(CONFIG_BL31_LOAD_ADDR);

    printk(LOG_LEVEL_INFO, "Back to SyterKit\n");

    // if kernel boot not success, jump to fel.
    jmp_to_fel();
    return 0;
}

const msh_command_entry commands[] = {
        msh_define_command(boot),
        msh_command_end,
};

static int abortboot_single_key(int bootdelay) {
    int abort = 0;
    unsigned long ts;

    printk(LOG_LEVEL_INFO, "Hit any key to stop autoboot: %2d ", bootdelay);

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
    uint64_t dram_size = sunxi_dram_init(NULL);

    sunxi_clk_dump();

    arm32_mmu_enable(SDRAM_BASE, dram_size);

    /* Initialize the small memory allocator. */
    smalloc_init(CONFIG_HEAP_BASE, CONFIG_HEAP_SIZE);

    sunxi_nsi_init();

    /* Clear the image_info_t struct. */
    memset(&image, 0, sizeof(image_info_t));

    image.bl31_dest = (uint8_t *) CONFIG_BL31_LOAD_ADDR;
    image.scp_dest = (uint8_t *) CONFIG_SCP_LOAD_ADDR;
    image.extlinux_dest = (uint8_t *) CONFIG_EXTLINUX_LOAD_ADDR;
    image.of_dest = (uint8_t *) CONFIG_DTB_LOAD_ADDR;
    image.ramdisk_dest = (uint8_t *) CONFIG_INITRD_LOAD_ADDR;
    image.kernel_dest = (uint8_t *) CONFIG_KERNEL_LOAD_ADDR;

    strcpy(image.bl31_filename, CONFIG_BL31_FILENAME);
    strcpy(image.scp_filename, CONFIG_SCP_FILENAME);
    strcpy(image.extlinux_filename, CONFIG_EXTLINUX_FILENAME);

    /* Initialize the SD host controller. */
    if (sunxi_sdhci_init(&sdhci0) != 0) {
        printk(LOG_LEVEL_ERROR, "SMHC: %s controller init failed\n", sdhci0.name);
        goto _shell;
    } else {
        printk(LOG_LEVEL_INFO, "SMHC: %s controller initialized\n", sdhci0.name);
    }

    /* Initialize the SD card and check if initialization is successful. */
    if (sdmmc_init(&card0, &sdhci0) != 0) {
        printk(LOG_LEVEL_WARNING, "SMHC: init failed, Retrying...\n");
        mdelay(30);
        if (sdmmc_init(&card0, &sdhci0) != 0) {
            printk(LOG_LEVEL_WARNING, "SMHC: init failed\n");
            goto _shell;
        }
    }

    /* Load the DTB, kernel image, and configuration data from the SD card. */
    if (load_sdcard(&image) != 0) {
        printk(LOG_LEVEL_WARNING, "SMHC: loading failed\n");
        goto _shell;
    }

    if (load_extlinux(&image, dram_size) != 0) {
        printk(LOG_LEVEL_ERROR, "EXTLINUX: load extlinux failed\n");
        goto _shell;
    }

    printk(LOG_LEVEL_INFO, "EXTLINUX: load extlinux done, now booting...\n");

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