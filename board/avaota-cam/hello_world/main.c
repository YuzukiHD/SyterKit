/* SPDX-License-Identifier: Apache-2.0 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <types.h>

#include <log.h>

#include <sys-clk.h>
#include <sys-dram.h>

extern sunxi_serial_t uart_dbg;
extern dram_para_t dram_para;

int main(void) {
    sunxi_serial_init(&uart_dbg);

    show_banner();

    printk_info("Hello World!\n");

    sunxi_clk_init();

    printk_info("clk init finish\n");

    sunxi_clk_dump();

    sunxi_dram_init(&dram_para);

    abort();

    return 0;
}