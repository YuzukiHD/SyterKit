/* SPDX-License-Identifier: Apache-2.0 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>

extern sunxi_serial_t uart_dbg;

int main(void) {
    sunxi_serial_init(&uart_dbg);

    printk_info("Hello World!\n");

    sunxi_clk_init();

    while (1) {
        printk_info("Hello World!\n");
    }

    printk_info("Hello World!\n");

    return 0;
}