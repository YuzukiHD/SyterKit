/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>
#include <io.h>

#include <log.h>

#include <common.h>

#include <dt-bindings/soc/sun252iw2.h>
#include <drivers/clk/clk.h>

#include <mmu.h>

#include <drivers/dma/dma.h>
#include <drivers/gpio/gpio.h>
#include <drivers/i2c/i2c.h>
#include <drivers/pwm/pwm.h>
#include <drivers/sid/sid.h>
#include <drivers/spi/spi.h>
#include <dt-compatible/sid-dt.h>

void show_chip()
{
	sunxi_sid_t sid;
	uint32_t chip_sid[4];

	if (sunxi_sid_dt_read_alias(&sid, "sid0") != DRIVER_OK) {
		printk_error("SID: invalid devicetree configuration\n");
		return;
	}
	chip_sid[0] = sunxi_efuse_read(&sid, 0x0U);
	chip_sid[1] = sunxi_efuse_read(&sid, 0x4U);
	chip_sid[2] = sunxi_efuse_read(&sid, 0x8U);
	chip_sid[3] = sunxi_efuse_read(&sid, 0xcU);

	printk_info("Model: Yuzuki Neko board.\n");
#if __riscv_xlen == 32
	printk_info("Core: XuanTie C907 RISC-V ILP32 Core.\n");
#else
	printk_info("Core: XuanTie C906 RISC-V LP64D Core.\n");
#endif
	printk_info("Chip SID = %08x%08x%08x%08x\n", chip_sid[0], chip_sid[1], chip_sid[2], chip_sid[3]);
}

void sys_reset(void)
{
	write32(SUNXI_RISCV_WDG_BASE + 0x08, 0x16aa0001U);

	for (;;) {
	}
}
