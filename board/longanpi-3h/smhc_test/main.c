/* SPDX-License-Identifier: Apache-2.0 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>

#include <common.h>

#include <mmu.h>

#include <sys-dram.h>
#include <sys-gpio.h>
#include <sys-i2c.h>
#include <sys-sid.h>
#include <sys-sdcard.h>
#include <sys-spi.h>
#include <sys-uart.h>

extern sunxi_serial_t uart_dbg;

extern sunxi_i2c_t i2c_pmu;

extern void set_cpu_poweroff(void);

int main(void) {
    sunxi_serial_init(&uart_dbg);

    show_banner();

    sunxi_clk_init();

    sunxi_clk_dump();

    set_cpu_poweroff();

    neon_enable();

    pmu_axp1530_init(&i2c_pmu);

    pmu_axp1530_dump(&i2c_pmu);

    int set_vol = 1100; /* LPDDR4 1100mv */

    int temp_vol, src_vol = pmu_axp1530_get_vol(&i2c_pmu, "dcdc3");
    if (src_vol > set_vol) {
        for (temp_vol = src_vol; temp_vol >= set_vol; temp_vol -= 50) {
            pmu_axp1530_set_vol(&i2c_pmu, "dcdc3", temp_vol, 1);
        }
    } else if (src_vol < set_vol) {
        for (temp_vol = src_vol; temp_vol <= set_vol; temp_vol += 50) {
            pmu_axp1530_set_vol(&i2c_pmu, "dcdc3", temp_vol, 1);
        }
    }

    mdelay(30); /* Delay 300ms for pmu bootup */

    pmu_axp1530_dump(&i2c_pmu);

    printk(LOG_LEVEL_INFO, "DRAM: DRAM Size = %dMB\n", sunxi_dram_init());

    sunxi_clk_dump();

    /* Initialize the SD host controller. */
    if (sunxi_sdhci_init(&sdhci0) != 0) {
        printk(LOG_LEVEL_ERROR, "SMHC: %s controller init failed\n", sdhci0.name);
    } else {
        printk(LOG_LEVEL_INFO, "SMHC: %s controller v%x initialized\n", sdhci0.name, sdhci0.reg->vers);
    }

    abort();

    return 0;
}