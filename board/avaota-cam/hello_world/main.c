/* SPDX-License-Identifier: Apache-2.0 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <types.h>

#include <log.h>

#include <sys-clk.h>
#include <sys-dma.h>
#include <sys-dram.h>
#include <sys-i2c.h>

#include <common.h>

#include <cli.h>
#include <cli_shell.h>
#include <cli_termesc.h>

extern sunxi_serial_t uart_dbg;
extern dram_para_t dram_para;
extern sunxi_dma_t sunxi_dma;
extern sunxi_i2c_t sunxi_i2c0;

msh_declare_command(helloworld);

msh_define_help(helloworld, "display helloworld", "Usage: helloworld\n");
int cmd_helloworld(int argc, const char **argv) {
    printk(LOG_LEVEL_MUTE, "Hello World!\n");
    return 0;
}

const msh_command_entry commands[] = {
        msh_define_command(helloworld),
        msh_command_end,
};

int main(void) {
    sunxi_clk_pre_init();

    sunxi_serial_init(&uart_dbg);

    // show_banner();

    printk_info("Hello World!\n");

    sunxi_clk_init();

    printk_info("CLK init finish\n");

    sunxi_clk_dump();

    uint64_t dram_size = sunxi_dram_init(&dram_para);

    sunxi_dma_init(&sunxi_dma);

    sunxi_dma_test((uint32_t *) 0x81008000, (uint32_t *) 0x80008000);

    sunxi_i2c_init(&sunxi_i2c0);

    uint8_t axp_val = 0;

    sunxi_i2c_read(&sunxi_i2c0, 0x37, 0x00, &axp_val);

    syterkit_shell_attach(commands);

    abort();

    return 0;
}