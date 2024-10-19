/* SPDX-License-Identifier: Apache-2.0 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <types.h>

#include <log.h>

#include <sys-clk.h>
#include <sys-dram.h>

#include <common.h>

#include <cli.h>
#include <cli_shell.h>
#include <cli_termesc.h>

extern sunxi_serial_t uart_dbg;
extern dram_para_t dram_para;

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
    sunxi_serial_init(&uart_dbg);

    show_banner();

    printk_info("Hello World!\n");

    sunxi_clk_init();

    printk_info("clk init finish\n");

    sunxi_clk_dump();

    uint64_t dram_size = sunxi_dram_init(&dram_para);

    syterkit_shell_attach(commands);

    abort();

    return 0;
}