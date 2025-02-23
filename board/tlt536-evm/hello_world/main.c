/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>

#include <common.h>

#include <mmc/sys-sdhci.h>

#include <sys-dram.h>
#include <sys-sdcard.h>
#include <sys-i2c.h>

#include <cli.h>
#include <cli_shell.h>
#include <cli_termesc.h>

extern sunxi_serial_t uart_dbg;

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

    sunxi_clk_init();

    sunxi_clk_dump();

    printk_info("Hello World!\n");

    syterkit_shell_attach(commands);

    abort();

    return 0;
}