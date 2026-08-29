/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file board.c
 * @brief Board support for the Avaota F1 (sun300iw1).
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>
#include <io.h>

#include <log.h>

#include <common.h>

#include <drivers/clk/sun300iw1/reg.h>
#include <dt-bindings/soc/sun300iw1.h>
#include <drivers/clk/clk.h>

#include <mmu.h>

#include <drivers/dma/dma.h>
#include <drivers/dram/dram.h>
#include <drivers/gpio/gpio.h>
#include <drivers/i2c/i2c.h>
#include <drivers/sid/sid.h>
#include <drivers/spi/spi.h>
#include <dt-compatible/sid-dt.h>
#include <e907/sysmap.h>

/**
 * @brief Print the SoC identification banner for the Avaota F1 board.
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

	pr_info("Model: AvaotaSBC Avaota F1 board.\n");
	pr_info("Core: XuanTie E907 RISC-V Core.\n");
	pr_info("Chip SID = %08x%08x%08x%08x\n", chip_sid[0], chip_sid[1], chip_sid[2], chip_sid[3]);
}

/**
 * @brief Detect the high-speed oscillator frequency.
 *
 * Triggers and waits for a frequency detection, then reports whether the
 * oscillator is running at 24 MHz or 40 MHz, updating the global HOSC
 * frequency variable.
 *
 * @return HOSC_FREQ_24M when the oscillator runs at 24 MHz, otherwise
 *         HOSC_FREQ_40M.
 */
int sunxi_hosc_detect(void)
{
	uintptr_t detect_reg;
	uint32_t val;

	detect_reg = SUNXI_CCU_AON_BASE + HOSC_FREQ_DET;
	val = readl(detect_reg);

	writel(val & (~HOSC_FREQ_DET_HOSC_CLEAR_MASK), detect_reg);
	writel(val | HOSC_FREQ_DET_HOSC_ENABLE_DETECT, detect_reg);

	while (!(HOSC_FREQ_DET_HOSC_FREQ_READY_CLEAR_MASK & readl(detect_reg)))
		;

	val = (readl(detect_reg) & HOSC_FREQ_DET_HOSC_FREQ_DET_CLEAR_MASK) >> HOSC_FREQ_DET_HOSC_FREQ_DET_OFFSET;
	if (val < ((HOSC_24M_COUNTER + HOSC_40M_COUNTER) / 2)) {
		current_hosc_freq = HOSC_FREQ_24M;
		return HOSC_FREQ_24M;
	} else {
		current_hosc_freq = HOSC_FREQ_40M;
		return HOSC_FREQ_40M;
	}
}

/**
 * @brief Populate the system memory map with the SoC address regions.
 *
 * Registers the RAM and device address windows with the sysmap subsystem so
 * later stages can look up the memory attributes of each region.
 */
void sysmap_init(void)
{
	sysmap_add_mem_region(0x00000000, 0x10000000, SYSMAP_MEM_ATTR_RAM);
	sysmap_add_mem_region(0x10000000, 0x02000000, SYSMAP_MEM_ATTR_RAM);
	sysmap_add_mem_region(0x12000000, 0x1E000000, SYSMAP_MEM_ATTR_DEVICE);
	sysmap_add_mem_region(0x30000000, 0x10000000, SYSMAP_MEM_ATTR_DEVICE);
	sysmap_add_mem_region(0x40000000, 0x28000000, SYSMAP_MEM_ATTR_DEVICE);
	sysmap_add_mem_region(0x68000000, 0x01000000, SYSMAP_MEM_ATTR_DEVICE);
	sysmap_add_mem_region(0x69000000, 0x17000000, SYSMAP_MEM_ATTR_DEVICE);
	sysmap_add_mem_region(0x80000000, 0x7FFFFFFF, SYSMAP_MEM_ATTR_RAM);
}

/**
 * @brief Reset the system using the PRCM and RTC watchdog.
 *
 * Requests a software reset through the PRCM and programs the RTC watchdog
 * with the reset key, then spins forever while the SoC performs the reset.
 */
void sys_reset(void)
{
	setbits_le32(SUNXI_PRCM_BASE + 0x1c, 1U << 3);
	write32(SUNXI_RTC_WDG_BASE + 0x18, 0x16aa0000U);
	write32(SUNXI_RTC_WDG_BASE + 0x08, 0x16aa0001U);

	for (;;) {
	}
}
