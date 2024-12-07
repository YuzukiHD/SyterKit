/* SPDX-License-Identifier: Apache-2.0 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>

#include <common.h>

#include <mmc/sys-sdhci.h>

#include <sys-dram.h>
#include <sys-sdcard.h>
#include <sys-i2c.h>

extern sunxi_serial_t uart_dbg;

extern sunxi_i2c_t i2c_pmu;

extern sunxi_sdhci_t sdhci0;

int main(void) {
    sunxi_serial_init(&uart_dbg);

    show_banner();

    sunxi_i2c_init(&i2c_pmu);

    sunxi_clk_init();

    sunxi_clk_dump();

    pmu_axp8191_init(&i2c_pmu);

    pmu_axp8191_dump(&i2c_pmu);

    sunxi_dram_init(NULL);

    printk_info("Hello World!\n");

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

    abort();

    return 0;
}