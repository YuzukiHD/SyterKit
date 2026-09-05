/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file board.c
 * @brief Board support for the TLT153 Mini EVM (sun8iw22).
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>
#include <io.h>

#include <log.h>

#include <common.h>

#include <dt-bindings/soc/sun8iw22.h>
#include <drivers/clk/clk.h>

#include <mmu.h>

#include <drivers/dram/dram.h>
#include <drivers/gpio/gpio.h>
#include <drivers/i2c/i2c.h>
#include <drivers/sid/sid.h>
#include <dt-compatible/sid-dt.h>
#include <drivers/spi/spi.h>
#include <drivers/serial/serial.h>

/**
 * @brief Disable the MMU, caches, and interrupts before OS handoff.
 */
void clean_syterkit_data(void)
{
	/* Disable MMU, data cache, instruction cache, interrupts */
	arm32_mmu_disable();
	pr_info("disable mmu ok...\n");
	arm32_dcache_disable();
	pr_info("disable dcache ok...\n");
	arm32_icache_disable();
	pr_info("disable icache ok...\n");
	arm32_interrupt_disable();
	pr_info("free interrupt ok...\n");
}

/**
 * @brief Print the SoC identification banner for the TLT153 Mini EVM board.
 *
 * Reads the 128-bit chip SID from the eFuses through the devicetree SID
 * alias and prints the chip SID, chip type, and chip version to the console.
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

	pr_info("Chip SID = %08x%08x%08x%08x\n", chip_sid[0], chip_sid[1], chip_sid[2], chip_sid[3]);

	uint32_t chip_markid_sid = chip_sid[0] & 0xffff;

	switch (chip_markid_sid) {
	case 0x7700:
		pr_info("Chip type = T153MX-BCX");
		break;
	default:
		pr_info("Chip type = UNKNOW");
		break;
	}

	setbits_le32(SUNXI_SYSCTRL_BASE + 0x24, BIT(15));
	uint32_t version = (read32(SUNXI_SYSCTRL_BASE + 0x24) & 0xFFFF0007) >> 16;
	printk(LOG_LEVEL_MUTE, " Chip Version = 0x%04x \n", version);
}

/**
 * @brief Reset the system using the watchdog.
 *
 * Programs the watchdog with the reset key and then spins forever while the
 * SoC performs the reset.
 */
void sys_reset(void)
{
	write32(SUNXI_WDOG_BASE + 0x08, 0x16aa0001U);

	for (;;) {
	}
}
