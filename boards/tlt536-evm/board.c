/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file board.c
 * @brief Board support for the TLT536 EVM (sun55iw6).
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>
#include <io.h>

#include <log.h>

#include <common.h>

#include <dt-bindings/soc/sun55iw6.h>
#include <drivers/clk/clk.h>

#include <mmu.h>

#include <drivers/dram/dram.h>
#include <drivers/gpio/gpio.h>
#include <dt-compatible/gpio-dt.h>
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

#define GPIO_POW_MOD_SEL_MASK (0x033ffff3)
#define R_GPIO_POW_MOD_SEL_MASK (0xf)

/**
 * @brief Configure the PIO and R_PIO GPIO power mode select registers.
 *
 * Programs the power mode selection for the main PIO bank and the R_PIO bank
 * using the fixed board voltage configuration.
 */
void sunxi_gpio_power_mode_init(void)
{
	sunxi_gpio_t pio;
	sunxi_gpio_t r_pio;
	uint32_t reg_val;

	if (sunxi_gpio_dt_read_alias(&pio, "gpio0") != DRIVER_OK || sunxi_gpio_dt_read_alias(&r_pio, "gpio1") != DRIVER_OK) {
		pr_err("GPIO: invalid PIO devicetree configuration\n");
		return;
	}
	reg_val = readl(pio.base + 0x40);
	reg_val &= ~GPIO_POW_MOD_SEL_MASK;
	reg_val |= 0x022AAAA2;
	writel(reg_val, pio.base + 0x40);

	reg_val = readl(r_pio.base + 0x340);
	reg_val &= ~R_GPIO_POW_MOD_SEL_MASK;
	reg_val |= 0xA;
	writel(reg_val, r_pio.base + 0x340);
}

/**
 * @brief Print the SoC identification banner for the TLT536 EVM board.
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
	case 0x5f00:
		pr_info("Chip type = T536MX-CXX");
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
	write32(SUNXI_WDG_BASE + 0x08, 0x16aa0001U);

	for (;;) {
	}
}
