/* SPDX-License-Identifier: Apache-2.0 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>

#include <common.h>

#include "sys-i2c.h"
#include "sys-uart.h"

sunxi_serial_t uart_dbg = {
        .base = 0x02500000,
        .id = 0,
        .gpio_tx = {GPIO_PIN(GPIO_PORTH, 9), GPIO_PERIPH_MUX5},
        .gpio_rx = {GPIO_PIN(GPIO_PORTH, 10), GPIO_PERIPH_MUX5},
};

sunxi_i2c_t i2c_0 = {
        .base = 0x02502000,
        .id = 0,
        .speed = 4000000,
        .gpio_scl = {GPIO_PIN(GPIO_PORTE, 4), GPIO_PERIPH_MUX8},
        .gpio_sda = {GPIO_PIN(GPIO_PORTE, 5), GPIO_PERIPH_MUX8},
};

int main(void) {
    sunxi_serial_init(&uart_dbg);

    sunxi_clk_init();

    sunxi_i2c_init(&i2c_0);

    printk(LOG_LEVEL_INFO, "Hello World\n");

    int ret = 0;

    while (1) {
        printk(LOG_LEVEL_INFO, "sunxi_i2c_write\n");
        ret = sunxi_i2c_write(&i2c_0, 0x32, 0x11, 0x11);
        mdelay(100);
        printk(LOG_LEVEL_INFO, "sunxi_i2c_write done, ret = %08x\n", ret);
    }

    return 0;
}