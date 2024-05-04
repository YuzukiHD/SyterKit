/* SPDX-License-Identifier: Apache-2.0 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>
#include <mmu.h>

#include <common.h>

#include <pmu/axp.h>
#include <sys-dram.h>
#include <sys-i2c.h>

extern sunxi_serial_t uart_dbg;

extern sunxi_i2c_t i2c_pmu;

extern void set_rpio_power_mode(void);
extern void rtc_set_vccio_det_spare(void);

#include "memtester.c"


int main(void) {
    sunxi_serial_init(&uart_dbg);

    show_banner();

    sunxi_clk_init();

    sunxi_clk_dump();

    rtc_set_vccio_det_spare();

    set_rpio_power_mode();

    sunxi_i2c_init(&i2c_pmu);

    pmu_axp1530_init(&i2c_pmu);

    pmu_axp2202_init(&i2c_pmu);

    pmu_axp2202_set_vol(&i2c_pmu, "dcdc3", 1160, 1);

    mdelay(30); /* Delay 300ms for pmu bootup */

    pmu_axp1530_dump(&i2c_pmu);

    pmu_axp2202_dump(&i2c_pmu);

    enable_sram_a3();

    uint64_t dram_size = sunxi_dram_init(NULL);

    printk_info("DRAM: DRAM Size = %dMB\n", dram_size);

    sunxi_clk_dump();

    do_memtester(SDRAM_BASE, dram_size * 1024 * 1024);

    int i = 0;

    while (1) {
        i++;
        printk_info("Count: %d\n", i);
        mdelay(1000);
    }

    abort();

    return 0;
}