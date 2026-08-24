/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>
#include <drivers/clk/clk.h>

extern sunxi_serial_t uart_dbg;

int main(void)
{
	sunxi_clk_init();

	printk_info("Hello World!\n");

	return 0;
}