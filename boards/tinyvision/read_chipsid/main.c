/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>
#include <dt-compatible/ccu-dt.h>

#include <common.h>

#include <drivers/soc/sid.h>
#include <dt-compatible/sid-dt.h>

extern sunxi_serial_t uart_dbg;

int main(void) {
	sunxi_ccu_t ccu;
	sunxi_sid_t sid;

	if (sunxi_ccu_dt_read(&ccu) != DRIVER_OK) {
		printk_error("CCU: invalid devicetree configuration\n");
		return -1;
	}

	sunxi_clk_init(&ccu);
	if (sunxi_sid_dt_read_alias(&sid, "sid0") != DRIVER_OK) {
		printk_error("SID: invalid devicetree configuration\n");
		return -1;
	}

	sunxi_efuse_dump(&sid);

	return 0;
}
