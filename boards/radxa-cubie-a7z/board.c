/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file board.c
 * @brief Board support for the Radxa Cubie A7Z (sun60iw2).
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>
#include <io.h>

#include <log.h>

#include <common.h>

#include <dt-bindings/soc/sun60iw2.h>
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
 * @enum sunxi_soc_version_t
 * @brief SoC silicon revision identifiers.
 */
typedef enum {
	SUNXI_SOC_VER_INVALID = -1, /**< Invalid or unknown silicon revision. */
	SUNXI_SOC_VER_A = 0, /**< Silicon revision A. */
	SUNXI_SOC_VER_B = 1, /**< Silicon revision B. */
	SUNXI_SOC_VER_C = 2, /**< Silicon revision C. */
} sunxi_soc_version_t;

/**
 * @brief Read the SoC silicon version from the version register.
 *
 * @return The detected SoC revision as a sunxi_soc_version_t value.
 */
static sunxi_soc_version_t sunxi_get_soc_ver(void)
{
	uint32_t value;

	value = readl(SUNXI_SOC_VER_REG);
	value &= SUNXI_SOC_VER_MASK;

	return SUNXI_SOC_VER_A + value;
}

/**
 * @brief Program the PLL LDO voltage for the detected silicon revision.
 *
 * @param[in] version SoC silicon revision reported by sunxi_get_soc_ver().
 */
static void sunxi_pll_ldo_init(sunxi_soc_version_t version)
{
	if (version == SUNXI_SOC_VER_A) {
		writel(0xA7070025, PLL_LDO_REG);
		writel(0xA7070025, PLL_LDO_REG);
	} else if (version == SUNXI_SOC_VER_B) {
		writel(0xA7060025, PLL_LDO_REG);
		writel(0xA7060025, PLL_LDO_REG);
	}
}

/**
 * @brief Perform common board initialization.
 *
 * Configures the GPIO power mode for revision-B silicon and programs the
 * PLL LDO for the detected silicon revision.
 */
void board_common_init(void)
{
	sunxi_gpio_t pio;
	sunxi_soc_version_t version = sunxi_get_soc_ver();

	if (version == SUNXI_SOC_VER_B) {
		if (sunxi_gpio_dt_read_alias(&pio, "gpio0") != DRIVER_OK) {
			pr_err("GPIO: invalid PIO devicetree configuration\n");
			return;
		}
		writel(0x01155550, pio.base + GPIO_POW_MODE_REG);
	}

	sunxi_pll_ldo_init(version);
}

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
 * @brief Print the SoC identification banner for the Radxa Cubie board.
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
		pr_err("SID: invalid devicetree configuration\n");
		return;
	}
	chip_sid[0] = sunxi_efuse_sram_read(&sid, 0x0U);
	chip_sid[1] = sunxi_efuse_sram_read(&sid, 0x4U);
	chip_sid[2] = sunxi_efuse_sram_read(&sid, 0x8U);
	chip_sid[3] = sunxi_efuse_sram_read(&sid, 0xcU);

	pr_info("Model: Radxa Cubie A7A board.\n");
	pr_info("Core: Arm Dual-Core Cortex-A76 + Arm Hexa-Core Cortex-A55\n");
	pr_info("Chip SID = %08x%08x%08x%08x\n", chip_sid[0], chip_sid[1], chip_sid[2], chip_sid[3]);

	uint32_t chip_markid_sid = chip_sid[0] & 0xffff;

	switch (chip_markid_sid) {
	case 0x5100:
		pr_info("Chip type = A733MX-HN3");
		break;
	case 0x5f00:
		pr_info("Chip type = A733MX-N3X");
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
	write32(SUNXI_WDT0_BASE + 0x08, 0x16aa0001U);

	for (;;) {
	}
}
