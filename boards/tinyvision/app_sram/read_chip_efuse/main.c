/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>
#include <drivers/clk/clk.h>
#include <drivers/sid/sid.h>
#include <dt-compatible/sid-dt.h>

#include <common.h>

extern sunxi_serial_t uart_dbg;

int main(void)
{
	sunxi_sid_t sid;

	if (sunxi_sid_dt_read_alias(&sid, "sid0") != DRIVER_OK) {
		pr_err("Board: invalid devicetree configuration\n");
		return -1;
	}

	sunxi_clk_init();

	uint32_t id[4];

	id[0] = sunxi_efuse_sram_read(&sid, 0x0U);
	id[1] = sunxi_efuse_sram_read(&sid, 0x4U);
	id[2] = sunxi_efuse_sram_read(&sid, 0x8U);
	id[3] = sunxi_efuse_sram_read(&sid, 0xcU);

	pr_info("Chip ID is: %08x%08x%08x%08x\n", id[0], id[1], id[2], id[3]);

	return 0;
}
