/* SPDX-License-Identifier: Apache-2.0 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>

#include <common.h>

#include "sys-uart.h"

#include "oled.h"

sunxi_serial_t uart_dbg = {
        .base = 0x02500000,
        .id = 0,
        .gpio_tx = {GPIO_PIN(PORTH, 9), GPIO_PERIPH_MUX5},
        .gpio_rx = {GPIO_PIN(PORTH, 10), GPIO_PERIPH_MUX5},
};

int main(void) {
    sunxi_serial_init(&uart_dbg);

    sunxi_clk_init();

    printk(LOG_LEVEL_INFO, "Hello World\n");

    OLED_Init();

    OLED_ShowString(12, 16, "SyterKit", 16, 1);
	OLED_ShowString(20, 32, "I2C OLED", 16, 1);

    OLED_Refresh();

    abort();

    return 0;
}