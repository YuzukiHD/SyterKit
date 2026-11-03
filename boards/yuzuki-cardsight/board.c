/* SPDX-License-Identifier: MIT */

#include <stdint.h>
#include <common.h>
#include <io.h>
#include <log.h>
#include <drivers/sid/sid.h>
#include <dt-compatible/sid-dt.h>
#include <dt-bindings/soc/sun252iw1.h>

void show_chip(void)
{
	sunxi_sid_t sid;
	uint32_t chip_sid[4];

	pr_info("Model: Yuzuki CardSight (sun252iw1, PSRAM).\n");
#ifdef CONFIG_ARCH_CPU_C907
	pr_info("Core: XuanTie C907 RV32 RISC-V Core.\n");
#else
	pr_info("Core: XuanTie E907 RISC-V Core.\n");
#endif
	if (sunxi_sid_dt_read_alias(&sid, "sid0") != DRIVER_OK) {
		pr_err("SID: invalid devicetree configuration\n");
		return;
	}
	for (unsigned int i = 0; i < 4; i++)
		chip_sid[i] = sunxi_efuse_sram_read(&sid, i * sizeof(uint32_t));
	pr_info("Chip SID = %08x%08x%08x%08x\n", chip_sid[0], chip_sid[1], chip_sid[2], chip_sid[3]);
}

void sys_reset(void)
{
	writel(0x16aa0001U, SUNXI_CPUX_WDG_BASE + 0x08);
	for (;;) {
	}
}
