/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>

#include <common.h>

extern sunxi_serial_t uart_dbg;

int main(void) {
	sunxi_serial_init(&uart_dbg);

	sunxi_clk_init();

	uint32_t id[4];

	id[0] = read32(0x03006200 + 0x0);
	id[1] = read32(0x03006200 + 0x4);
	id[2] = read32(0x03006200 + 0x8);
	id[3] = read32(0x03006200 + 0xc);

	printk_info("Chip ID is: %08x%08x%08x%08x\n", id[0], id[1], id[2], id[3]);

	return 0;
}