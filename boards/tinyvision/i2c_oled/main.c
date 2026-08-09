/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>
#include <dt-compatible/ccu-dt.h>

#include <common.h>

#include <drivers/serial.h>

#include "oled.h"

extern sunxi_serial_t uart_dbg;

int main(void) {
	sunxi_ccu_t ccu;

	if (sunxi_ccu_dt_read(&ccu) != DRIVER_OK) {
		printk_error("CCU: invalid devicetree configuration\n");
		return -1;
	}

	sunxi_clk_init(&ccu);

	printk_info("Hello World\n");

	if (OLED_Init() != 0)
		return -1;

	OLED_ShowString(12, 16, "SyterKit", 16, 1);
	OLED_ShowString(20, 32, "I2C OLED", 16, 1);

	OLED_Refresh();

	abort();

	return 0;
}
