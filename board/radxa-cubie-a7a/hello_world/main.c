/* SPDX-License-Identifier: GPL-2.0+ */

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
#include <sys-spi.h>

extern sunxi_serial_t uart_dbg;
extern sunxi_i2c_t i2c_pmu;
extern sunxi_spi_t sunxi_spi0;
extern sunxi_sdhci_t sdhci0;

int main(void) {
    sunxi_serial_init(&uart_dbg);

    show_banner();

    sunxi_i2c_init(&i2c_pmu);

    sunxi_clk_init();

    sunxi_clk_dump();

    pmu_axp8191_init(&i2c_pmu);

    pmu_axp8191_dump(&i2c_pmu);

    // TODO
    sunxi_spi_init(&sunxi_spi0);

    printk_info("Hello World!\n");

    abort();

    return 0;
}