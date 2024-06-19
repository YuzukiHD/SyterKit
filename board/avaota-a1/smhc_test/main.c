/* SPDX-License-Identifier: Apache-2.0 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>

#include <common.h>

#include <ff.h>
#include <mmu.h>

#include <cli.h>
#include <cli_shell.h>
#include <cli_termesc.h>

#include <sys-dram.h>
#include <sys-gpio.h>
#include <sys-i2c.h>
#include <sys-sdcard.h>
#include <sys-sid.h>
#include <sys-spi.h>
#include <sys-uart.h>

extern sunxi_serial_t uart_dbg;

extern sunxi_sdhci_t sdhci0;
extern sunxi_sdhci_t sdhci2;
extern uint32_t dram_para[32];
extern sunxi_i2c_t i2c_pmu;

#define CONFIG_SDMMC_SPEED_TEST_SIZE 1024// (unit: 512B sectors)

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

msh_declare_command(reload);
msh_define_help(reload, "rescan TF Card and reload DTB, Kernel zImage", "Usage: reload\n");
int cmd_reload(int argc, const char **argv) {
    if (sdmmc_init(&card0, &sdhci0) != 0) {
        printk_error("SMHC: init failed\n");
        return 0;
    }
    return 0;
}

msh_declare_command(read);
msh_define_help(read, "test", "Usage: read\n");
int cmd_read(int argc, const char **argv) {
    uint32_t start;
    uint32_t test_time;

    printk_debug("Clear Buffer data\n");
    memset((void *) SDRAM_BASE, 0x00, 0x2000);
    dump_hex(SDRAM_BASE, 0x100);

    printk_debug("Read data to buffer data\n");

    start = time_ms();
    sdmmc_blk_read(&card0, (uint8_t *) (SDRAM_BASE), 0, 1024);
    test_time = time_ms() - start;
    printk_debug("SDMMC: speedtest %uKB in %ums at %uKB/S\n",
                 (CONFIG_SDMMC_SPEED_TEST_SIZE * 512) / 1024, test_time,
                 (CONFIG_SDMMC_SPEED_TEST_SIZE * 512) / test_time);
    dump_hex(SDRAM_BASE, 0x100);
    return 0;
}

msh_declare_command(write);
msh_define_help(write, "test", "Usage: write\n");
int cmd_write(int argc, const char **argv) {
    uint32_t start;
    uint32_t test_time;

    printk_debug("Set Buffer data\n");
    memset((void *) SDRAM_BASE, 0x00, 0x2000);
    memcpy((void *) SDRAM_BASE, argv[1], strlen(argv[1]));

    start = time_ms();
    sdmmc_blk_write(&card0, (uint8_t *) (SDRAM_BASE), 0, 1024);
    test_time = time_ms() - start;
    printk_debug("SDMMC: speedtest %uKB in %ums at %uKB/S\n",
                 (CONFIG_SDMMC_SPEED_TEST_SIZE * 512) / 1024, test_time,
                 (CONFIG_SDMMC_SPEED_TEST_SIZE * 512) / test_time);
    return 0;
}

const msh_command_entry commands[] = {
        msh_define_command(reload),
        msh_define_command(read),
        msh_define_command(write),
        msh_command_end,
};

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

    pmu_axp2202_dump(&i2c_pmu);
    pmu_axp1530_dump(&i2c_pmu);

    sunxi_clk_set_cpu_pll(1416);

    enable_sram_a3();

    /* Initialize the DRAM and enable memory management unit (MMU). */
    uint64_t dram_size = sunxi_dram_init(&dram_para);

    printk_debug("DRAM Size = %dM\n", dram_size);

    sunxi_clk_dump();

    arm32_mmu_enable(SDRAM_BASE, dram_size);

    /* Initialize the SD host controller. */
    if (sunxi_sdhci_init(&sdhci0) != 0) {
        printk_error("SMHC: %s controller init failed\n", sdhci0.name);
    } else {
        printk_info("SMHC: %s controller initialized\n", sdhci0.name);
    }

    /* Initialize the SD card and check if initialization is successful. */
    if (sdmmc_init(&card0, &sdhci0) != 0) {
        printk_warning("SMHC: init failed\n");
    } else {
        printk_debug("Card OK!\n");
    }

    syterkit_shell_attach(commands);

    return 0;
}