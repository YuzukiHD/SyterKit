/* SPDX-License-Identifier: Apache-2.0 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <mmu.h>
#include <log.h>

extern sunxi_serial_t uart_dbg;

int main(void) {
    sunxi_serial_init(&uart_dbg);

    sunxi_clk_init();

    printk(LOG_LEVEL_INFO, "Hello World!\n");
    asm volatile("swi 0x1");

    dump_hex(0x000287e0, 0x2000);

    asm volatile(".word 0xfea00a00");

    printk(LOG_LEVEL_INFO, "Hello World! OFF\n");

    return 0;
}