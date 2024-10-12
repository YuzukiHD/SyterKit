/* SPDX-License-Identifier: Apache-2.0 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <types.h>

#include <log.h>

#include <sys-clk.h>

extern sunxi_serial_t uart_dbg;

int main(void) {
    sunxi_serial_init(&uart_dbg);

    show_banner();

    printk_info("Hello World!\n");

    sunxi_clk_init();

    printk_info("clk init finish\n");

    sunxi_clk_dump();

    printk_info("test Store address misaligned exception\n");

    asm volatile(".word 0x23232323");

    abort();

    return 0;
}