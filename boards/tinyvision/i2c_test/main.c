/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>
#include <drivers/clk/clk.h>

#include <common.h>

#include <drivers/i2c/i2c.h>
#include <drivers/serial/serial.h>
#include <dt-compatible/i2c-dt.h>

extern sunxi_serial_t uart_dbg;

int main(void) {
	sunxi_i2c_t i2c;

	if (sunxi_i2c_dt_read_alias(&i2c, "i2c0") != DRIVER_OK) {
		printk_error("I2C: invalid devicetree configuration\n");
		return -1;
	}


	sunxi_clk_init();

	sunxi_i2c_init(&i2c);

	printk_info("Hello World\n");

	int ret = 0;

	while (1) {
		printk_info("sunxi_i2c_write\n");
		ret = sunxi_i2c_write(&i2c, 0x32, 0x11, 0x11);
		mdelay(100);
		printk_info("sunxi_i2c_write done, ret = %08x\n", ret);
	}

	return 0;
}
