/* SPDX-License-Identifier: Apache-2.0 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>

#include <common.h>

#include <sys-dram.h>

extern sunxi_serial_t uart_dbg;

int main(void) {
    sunxi_serial_init(&uart_dbg);

    show_banner();

    sunxi_clk_init();

    sunxi_clk_dump();

    printk_info("Hello World!\n");

    abort();

    return 0;
}