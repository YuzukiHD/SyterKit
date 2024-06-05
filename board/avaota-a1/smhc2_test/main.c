/* SPDX-License-Identifier: Apache-2.0 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>

#include <common.h>

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

#define CONFIG_SDMMC_SPEED_TEST_SIZE 10240

extern sunxi_serial_t uart_dbg;

extern sunxi_i2c_t i2c_pmu;

extern sunxi_sdhci_t sdhci2;

extern uint32_t dram_para[32];

msh_declare_command(speedtest);
msh_define_help(speedtest, "Do speed test", "Usage: speedtest\n");
int cmd_speedtest(int argc, const char **argv) {
    uint32_t start;
    uint32_t test_time;

    // start = time_ms();
    // sdmmc_blk_write(&card0, (uint8_t *) (SDRAM_BASE), 0, CONFIG_SDMMC_SPEED_TEST_SIZE);
    // test_time = time_ms() - start;
    // printk_info("SDMMC: Write speedtest %uKB in %ums at %uKB/S\n",
    //        (CONFIG_SDMMC_SPEED_TEST_SIZE * 512) / 1024, test_time,
    //        (CONFIG_SDMMC_SPEED_TEST_SIZE * 512) / test_time);

    start = time_ms();
    sdmmc_blk_read(&card0, (uint8_t *) (SDRAM_BASE), 0, CONFIG_SDMMC_SPEED_TEST_SIZE);
    test_time = time_ms() - start;
    printk_info("SDMMC: Read speedtest %uKB in %ums at %uKB/S\n",
           (CONFIG_SDMMC_SPEED_TEST_SIZE * 512) / 1024, test_time,
           (CONFIG_SDMMC_SPEED_TEST_SIZE * 512) / test_time);

    return 0;
}

msh_declare_command(swi);
msh_define_help(swi, "Software interrupt test", "Usage: swi\n");
int cmd_swi(int argc, const char **argv) {
    asm volatile("svc #0");
    return 0;
}

const msh_command_entry commands[] = {
        msh_define_command(speedtest),
        msh_define_command(swi),
        msh_command_end,
};

int main(void) {
    sunxi_serial_init(&uart_dbg);

    show_banner();

    sunxi_clk_init();

    sunxi_clk_dump();

    sunxi_i2c_init(&i2c_pmu);

    pmu_axp1530_init(&i2c_pmu);

    enable_sram_a3();

    printk_info("DRAM: DRAM Size = %dMB\n", sunxi_dram_init(&dram_para));

    sunxi_clk_dump();

    /* Initialize the SD host controller. */
    if (sunxi_sdhci_init(&sdhci2) != 0) {
        printk_error("SMHC: %s controller init failed\n", sdhci2.name);
    } else {
        printk_info("SMHC: %s controller initialized\n", sdhci2.name);
    }

    /* Initialize the SD card and check if initialization is successful. */
    if (sdmmc_init(&card0, &sdhci2) != 0) {
        printk_warning("SMHC: init failed\n");
    } else {
        printk_debug("Card OK!\n");
    }

    syterkit_shell_attach(commands);

    return 0;
}