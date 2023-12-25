/* SPDX-License-Identifier: Apache-2.0 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>

#include <sys-dram.h>
#include <sys-sid.h>

#include <jmp.h>

extern sunxi_serial_t uart_dbg;
extern dram_para_t dram_para;

int main(void) {
    sunxi_serial_init(&uart_dbg);

    sunxi_clk_init();

    sunxi_dram_init(&dram_para);

    printk(LOG_LEVEL_INFO, "Hello World!\n");

    dump_efuse();

    sunxi_clk_reset();

    clean_syterkit_data();

    jmp_to_fel();

    return 0;
}