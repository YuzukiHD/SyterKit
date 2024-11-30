/* SPDX-License-Identifier: Apache-2.0 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <types.h>

#include <log.h>

#include <sys-clk.h>
#include <sys-dma.h>
#include <sys-dram.h>
#include <sys-gpio.h>
#include <sys-i2c.h>
#include <sys-sdcard.h>
#include <sys-sdhci.h>
#include <sys-spi-nand.h>
#include <sys-spi-nor.h>
#include <sys-spi.h>

#include <common.h>

#include <cli.h>
#include <cli_shell.h>
#include <cli_termesc.h>

extern sunxi_serial_t uart_dbg;
extern sunxi_serial_t uart_card;
extern dram_para_t dram_para;
extern sunxi_dma_t sunxi_dma;
extern sunxi_i2c_t sunxi_i2c0;
extern sunxi_spi_t sunxi_spi0;
extern sunxi_sdhci_t sdhci0;

#define CONFIG_SDMMC_SPEED_TEST_SIZE 1024// (unit: 512B sectors)
#define CHUNK_SIZE 0x20000

msh_declare_command(read);
msh_define_help(read, "read SMHC", "Usage: read\n");
int cmd_read(int argc, const char **argv) {
    uint32_t start;
    uint32_t test_time;

    printk_debug("Clear Buffer data\n");
    memset((void *) SDRAM_BASE, 0x00, 0x2000);
    dump_hex(SDRAM_BASE, 0x100);

    printk_debug("Read data to buffer data\n");

    start = time_ms();
    sdmmc_blk_read(&card0, (uint8_t *) (SDRAM_BASE), 0, CONFIG_SDMMC_SPEED_TEST_SIZE);
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
    sdmmc_blk_write(&card0, (uint8_t *) (SDRAM_BASE), 0, CONFIG_SDMMC_SPEED_TEST_SIZE);
    test_time = time_ms() - start;
    printk_debug("SDMMC: speedtest %uKB in %ums at %uKB/S\n",
                 (CONFIG_SDMMC_SPEED_TEST_SIZE * 512) / 1024, test_time,
                 (CONFIG_SDMMC_SPEED_TEST_SIZE * 512) / test_time);
    return 0;
}

msh_declare_command(load);
msh_define_help(load, "load SMHC", "Usage: load\n");
int cmd_load(int argc, const char **argv) {
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
    return 0;
}

msh_declare_command(dump);
msh_define_help(dump, "test", "Usage: dump\n");
int cmd_dump(int argc, const char **argv) {
    uart_printf("%s\n", (void *) (0x2020C00));
    return 0;
}

const msh_command_entry commands[] = {
        msh_define_command(load),
        msh_define_command(read),
        msh_define_command(write),
        msh_define_command(dump),
        msh_command_end,
};

int main(void) {
    sunxi_clk_pre_init();

    sunxi_serial_init(&uart_dbg);

    sunxi_serial_init(&uart_card);

    show_banner();

    printk_info("Hello World!\n");

    sunxi_clk_init();

    printk_info("CLK init finish\n");

    sunxi_clk_dump();

    uint32_t dram_size = sunxi_dram_init(&dram_para);

    printk_info("DRAM Size = %u\n", dram_size);

    printk_info("peri1x = %u\n", sunxi_clk_get_peri1x_rate());

    sunxi_spi_init(&sunxi_spi0);

    spi_nor_detect(&sunxi_spi0);

    memset((void *) 0x81000000, 0x0, 0x1000);

    uint32_t time = time_ms();
    spi_nor_read(&sunxi_spi0, (void *) 0x81000000, 0x0, 1024 * 1024 * 4);
    uint32_t time_end = time_ms();

    printk_debug("SPI: speedtest %uKB in %ums at %uKB/S\n",
                 1024 * 1024 * 4 / 1024, (time_end - time),
                 1024 * 1024 * 4 / (time_end - time));

    syterkit_shell_attach(commands);

    abort();

    return 0;
}