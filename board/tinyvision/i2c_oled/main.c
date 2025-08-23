/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>

#include <common.h>

#include "sys-uart.h"

#include "oled.h"

extern sunxi_serial_t uart_dbg;

int main(void) {
	sunxi_serial_init(&uart_dbg);

	sunxi_clk_init();

	printk_info("Hello World\n");

	OLED_Init();

	OLED_ShowString(12, 16, "SyterKit", 16, 1);
	OLED_ShowString(20, 32, "I2C OLED", 16, 1);

	OLED_Refresh();

	abort();

	return 0;
}