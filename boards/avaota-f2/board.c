/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file board.c
 * @brief Board support for the Avaota F2 (sun252iw1).
 */
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
#include <drivers/sid/sid.h>
#include <drivers/spi/spi.h>
#include <dt-compatible/sid-dt.h>

/**
 * @brief Print the SoC identification banner for the Avaota F2 board.
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
	chip_sid[0] = sunxi_efuse_sram_read(&sid, 0x0U);
	chip_sid[1] = sunxi_efuse_sram_read(&sid, 0x4U);
	chip_sid[2] = sunxi_efuse_sram_read(&sid, 0x8U);
	chip_sid[3] = sunxi_efuse_sram_read(&sid, 0xcU);

	pr_info("Model: AvaotaSBC Avaota F2 board.\n");
#ifdef CONFIG_ARCH_CPU_C907
#ifdef CONFIG_ARCH_RISCV32
	pr_info("Core: XuanTie C907 RV32 RISC-V Core.\n");
#else
	pr_info("Core: XuanTie C907 RV64 RISC-V Core.\n");
#endif
#else
	pr_info("Core: XuanTie E907 RISC-V Core.\n");
#endif
	pr_info("Chip SID = %08x%08x%08x%08x\n", chip_sid[0], chip_sid[1], chip_sid[2], chip_sid[3]);
}

/**
 * @brief Reset the system using the CPUX watchdog.
 *
 * Programs the watchdog with the reset key and then spins forever while the
 * SoC performs the reset.
 */
void sys_reset(void)
{
	write32(SUNXI_CPUX_WDG_BASE + 0x08, 0x16aa0001U);

	for (;;) {
	}
}
