/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file board.c
 * @brief Board support for the Avaota M1 (sun65iw1).
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>
#include <io.h>

#include <log.h>

#include <common.h>

#include <dt-bindings/soc/sun65iw1.h>
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
 * @brief Initialize the NSI (NoC) port priorities.
 *
 * Programs the AXI interconnect priority and attribute registers for the
 * boot-stage masters.
 *
 * @return 0 on success.
 */
int sunxi_nsi_init(void)
{
	writel(0x40005, 0x2402C00 + 0x6c);
	writel(0xFF, 0x2402600 + 0x14);
	writel(0xFF, 0x2402800 + 0x14);
	writel(0xFF, 0x2402a00 + 0x14);
	writel(0x1, 0x2400000 + 0xc);
	writel(0x1, 0x2400200 + 0xc);
	return 0;
}

/**
 * @brief Disable the MMU, caches, and interrupts before OS handoff.
 */
void clean_syterkit_data(void)
{
	/* Disable MMU, data cache, instruction cache, interrupts */
	arm32_mmu_disable();
	printk_info("disable mmu ok...\n");
	arm32_dcache_disable();
	printk_info("disable dcache ok...\n");
	arm32_icache_disable();
	printk_info("disable icache ok...\n");
	arm32_interrupt_disable();
	printk_info("free interrupt ok...\n");
}

/**
 * @brief Print the SoC identification banner for the Avaota M1 board.
 *
 * Reads the 128-bit chip SID from the eFuses through the devicetree SID
 * alias and prints the board model, CPU cores, chip SID, chip type, and
 * chip version to the console.
 */
void show_chip()
{
	sunxi_sid_t sid;
	uint32_t chip_sid[4];

	if (sunxi_sid_dt_read_alias(&sid, "sid0") != DRIVER_OK) {
		printk_error("SID: invalid devicetree configuration\n");
		return;
	}
	chip_sid[0] = sunxi_efuse_sram_read(&sid, 0x0U);
	chip_sid[1] = sunxi_efuse_sram_read(&sid, 0x4U);
	chip_sid[2] = sunxi_efuse_sram_read(&sid, 0x8U);
	chip_sid[3] = sunxi_efuse_sram_read(&sid, 0xcU);
	uint32_t chip_markid_sid = chip_sid[0] & 0xffff;

	printk_info("Model: Avaota M1 board.\n");
	printk_info("Core: Arm Dual-Core Cortex-A73 big Core\n");
	printk_info("\tArm Dual-Core Cortex-A53 Medium Core\n");
	printk_info("\tArm Quad-Core Cortex-A53 Little Core\n");
	printk_info("Chip SID = %08x%08x%08x%08x\n", chip_sid[0], chip_sid[1], chip_sid[2], chip_sid[3]);

	switch (chip_markid_sid) {
	case 0x5100:
		printk_info("Chip type = A537MX-0XX");
		break;
	default:
		printk_info("Chip type = UNKNOW");
		break;
	}

	setbits_le32(SUNXI_SYSCTRL_BASE + 0x24, BIT(15));
	uint32_t version = (read32(SUNXI_SYSCTRL_BASE + 0x24) & 0xFFFF0007) >> 16;
	printk(LOG_LEVEL_MUTE, " Chip Version = 0x%04x \n", version);
}

/**
 * @brief Reset the system using the CPUX watchdog.
 *
 * Programs the watchdog with the reset key and then spins forever while the
 * SoC performs the reset.
 */
void sys_reset(void)
{
	write32(SUNXI_WDT_CPUX_BASE + 0x08, 0x16aa0001U);

	for (;;) {
	}
}
