/* SPDX-License-Identifier: Apache-2.0 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>

#include <common.h>

#include <sys-i2c.h>
#include <sys-dram.h>

extern sunxi_serial_t uart_dbg;

extern sunxi_i2c_t i2c_pmu;

int main(void) {
    sunxi_serial_init(&uart_dbg);

    show_banner();

    sunxi_i2c_init(&i2c_pmu);

    sunxi_clk_init();

    printk_info("Hello World!\n");

    abort();

    return 0;
}