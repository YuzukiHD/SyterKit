/* SPDX-License-Identifier: Apache-2.0 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>

#include <common.h>
#include "sys-dram.h"

#include <config.h>

extern sunxi_serial_t uart_dbg;

extern dram_para_t dram_para;

int main(void) {
    sunxi_serial_init(&uart_dbg);

    show_banner();

    sunxi_clk_init();

    sunxi_dram_init(&dram_para);

    return 0;
}