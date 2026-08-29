/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file board.c
 * @brief Board support for the Yuzuki Neko (sun252iw2).
 */
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

/**
 * @brief Print the SoC identification banner for the Yuzuki Neko board.
 *
 * Reads the 128-bit chip SID from the eFuses through the devicetree SID
 * alias and prints the board model, CPU core, and chip SID to the console.
 */
void show_chip()
{
	sunxi_sid_t sid;
	uint32_t chip_sid[4];

	if (sunxi_sid_dt_read_alias(&sid, "sid0") != DRIVER_OK) {
		pr_err("SID: invalid devicetree configuration\n");
		return;
	}
	chip_sid[0] = sunxi_efuse_read(&sid, 0x0U);
	chip_sid[1] = sunxi_efuse_read(&sid, 0x4U);
	chip_sid[2] = sunxi_efuse_read(&sid, 0x8U);
	chip_sid[3] = sunxi_efuse_read(&sid, 0xcU);

	pr_info("Model: Yuzuki Neko board.\n");
#if __riscv_xlen == 32
	pr_info("Core: XuanTie C907 RISC-V ILP32 Core.\n");
#else
	pr_info("Core: XuanTie C907 RISC-V LP64D Core.\n");
#endif
	pr_info("Chip SID = %08x%08x%08x%08x\n", chip_sid[0], chip_sid[1], chip_sid[2], chip_sid[3]);
}

/**
 * @brief Reset the system using the RISC-V watchdog.
 *
 * Programs the watchdog with the reset key and then spins forever while the
 * SoC performs the reset.
 */
void sys_reset(void)
{
	write32(SUNXI_RISCV_WDG_BASE + 0x08, 0x16aa0001U);

	for (;;) {
	}
}
