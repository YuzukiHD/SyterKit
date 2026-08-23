/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>
#include <io.h>

#include <log.h>

#include <common.h>

#include <dt-bindings/soc/sun252iw1.h>
#include <drivers/clk/clk.h>

#include <mmu.h>

#include <drivers/mmc/sdhci.h>
#include <drivers/dma/dma.h>
#include <drivers/dram/dram.h>
#include <drivers/gpio/gpio.h>
#include <drivers/i2c/i2c.h>
#include <drivers/pwm/pwm.h>
#include <drivers/soc/sid.h>
#include <drivers/spi/spi.h>
#include <dt-compatible/sid-dt.h>

void show_chip() {
	sunxi_sid_t sid;
	uint32_t chip_sid[4];

	if (sunxi_sid_dt_read_alias(&sid, "sid0") != DRIVER_OK) {
		printk_error("SID: invalid devicetree configuration\n");
		return;
	}
	chip_sid[0] = sunxi_sid_read_sram(&sid, 0x0U);
	chip_sid[1] = sunxi_sid_read_sram(&sid, 0x4U);
	chip_sid[2] = sunxi_sid_read_sram(&sid, 0x8U);
	chip_sid[3] = sunxi_sid_read_sram(&sid, 0xcU);

	printk_info("Model: AvaotaSBC Avaota F2 board.\n");
	printk_info("Core: XuanTie E907 RISC-V Core.\n");
	printk_info("Chip SID = %08x%08x%08x%08x\n", chip_sid[0], chip_sid[1], chip_sid[2], chip_sid[3]);
	printk_info("dump brom log:\n");
	printk_info("==================================\n");
	printk_info("%s", (char *) 0x00120000);
	printk_info("==================================\n");
}

void sys_reset(void) {
	write32(SUNXI_CPUX_WDG_BASE + 0x08, 0x16aa0001U);

	for (;;) {
	}
}
