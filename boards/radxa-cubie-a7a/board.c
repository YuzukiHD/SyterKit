/* SPDX-License-Identifier: GPL-2.0+ */

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
#include <drivers/soc/sid.h>
#include <dt-compatible/sid-dt.h>
#include <drivers/spi/spi.h>
#include <drivers/serial/serial.h>

typedef enum {
	SUNXI_SOC_VER_INVALID = -1,
	SUNXI_SOC_VER_A = 0,
	SUNXI_SOC_VER_B = 1,
	SUNXI_SOC_VER_C = 2,
} sunxi_soc_version_t;

static sunxi_soc_version_t sunxi_get_soc_ver(void) {
	uint32_t value;

	value = readl(SUNXI_SOC_VER_REG);
	value &= SUNXI_SOC_VER_MASK;

	return SUNXI_SOC_VER_A + value;
}

static void sunxi_pll_ldo_init(sunxi_soc_version_t version) {
	if (version == SUNXI_SOC_VER_A) {
		writel(0xA7070025, PLL_LDO_REG);
		writel(0xA7070025, PLL_LDO_REG);
	} else if (version == SUNXI_SOC_VER_B) {
		writel(0xA7060025, PLL_LDO_REG);
		writel(0xA7060025, PLL_LDO_REG);
	}
}

void board_common_init(void) {
	sunxi_gpio_t pio;
	sunxi_soc_version_t version = sunxi_get_soc_ver();

	if (version == SUNXI_SOC_VER_B) {
		if (sunxi_gpio_dt_read_alias(&pio, "gpio0") != DRIVER_OK) {
			printk_error("GPIO: invalid PIO devicetree configuration\n");
			return;
		}
		writel(0x01155550, pio.base + GPIO_POW_MODE_REG);
	}

	sunxi_pll_ldo_init(version);
}

void clean_syterkit_data(void) {
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

	printk_info("Model: Radxa Cubie A7A board.\n");
	printk_info("Core: Arm Dual-Core Cortex-A76 + Arm Hexa-Core Cortex-A55\n");
	printk_info("Chip SID = %08x%08x%08x%08x\n", chip_sid[0], chip_sid[1], chip_sid[2], chip_sid[3]);

	uint32_t chip_markid_sid = chip_sid[0] & 0xffff;

	switch (chip_markid_sid) {
		case 0x5100:
			printk_info("Chip type = A733MX-HN3");
			break;
		case 0x5f00:
			printk_info("Chip type = A733MX-N3X");
			break;
		default:
			printk_info("Chip type = UNKNOW");
			break;
	}

	setbits_le32(SUNXI_SYSCTRL_BASE + 0x24, BIT(15));
	uint32_t version = (read32(SUNXI_SYSCTRL_BASE + 0x24) & 0xFFFF0007) >> 16;
	printk(LOG_LEVEL_MUTE, " Chip Version = 0x%04x \n", version);
}

void sys_reset(void) {
	write32(SUNXI_WDT0_BASE + 0x08, 0x16aa0001U);

	for (;;) {
	}
}
