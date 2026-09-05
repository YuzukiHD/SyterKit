/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>
#include <drivers/clk/clk.h>

#include <common.h>

#include <drivers/sid/sid.h>
#include <dt-compatible/sid-dt.h>

extern sunxi_serial_t uart_dbg;

int main(void)
{
	sunxi_sid_t sid;

	sunxi_clk_init();
	if (sunxi_sid_dt_read_alias(&sid, "sid0") != DRIVER_OK) {
		pr_err("SID: invalid devicetree configuration\n");
		return -1;
	}

	sunxi_efuse_dump(&sid);

	return 0;
}
