/* SPDX-License-Identifier: Apache-2.0 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>

extern sunxi_serial_t uart_dbg;

int main(void) {
    sunxi_serial_init(&uart_dbg);

    printk(LOG_LEVEL_INFO, "Hello World!\n");

    sunxi_clk_dump();

    sunxi_clk_init();

    sunxi_clk_dump();

    printk(LOG_LEVEL_INFO, "Hello World!\n");

    return 0;
}