/* SPDX-License-Identifier: Apache-2.0 */

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>

void __attribute__((weak)) sunxi_clk_init(void) {
    printk(LOG_LEVEL_WARNING, "sunxi_clk_init: not impl\n");
}

void __attribute__((weak)) sunxi_clk_reset(void) {
    printk(LOG_LEVEL_WARNING, "sunxi_clk_reset: not impl\n");
}

void __attribute__((weak)) sunxi_clk_dump(void) {
    printk(LOG_LEVEL_WARNING, "sunxi_clk_dump: not impl\n");
}

void __attribute__((weak)) sunxi_usb_clk_deinit(void) {
    printk(LOG_LEVEL_WARNING, "sunxi_usb_clk_deinit: not impl\n");
}

void __attribute__((weak)) sunxi_usb_clk_init(void) {
    printk(LOG_LEVEL_WARNING, "sunxi_usb_clk_init: not impl\n");
}

uint32_t __attribute__((weak)) sunxi_clk_get_peri1x_rate() {
    printk(LOG_LEVEL_WARNING, "sunxi_clk_get_peri1x_rate: not impl\n");
    return 0;
}

void __attribute__((weak)) sunxi_clk_set_cpu_pll(uint32_t freq) {
    printk(LOG_LEVEL_WARNING, "sunxi_clk_set_cpu_pll: not impl\n");
}