/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>

#include <common.h>

#include <sys-sid.h>

extern sunxi_serial_t uart_dbg;

int main(void) {
	sunxi_serial_init(&uart_dbg);

	sunxi_clk_init();

	syter_efuse_dump();

	return 0;
}