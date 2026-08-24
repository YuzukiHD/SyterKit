/* SPDX-License-Identifier: GPL-2.0+ */

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
#include <drivers/soc/sid.h>
#include <dt-compatible/sid-dt.h>
#include <drivers/spi/spi.h>
#include <drivers/serial/serial.h>

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

#define GPIO_POW_MOD_SEL_MASK (0x033ffff3)
#define R_GPIO_POW_MOD_SEL_MASK (0xf)

void sunxi_gpio_power_mode_init(void)
{
	sunxi_gpio_t pio;
	sunxi_gpio_t r_pio;
	uint32_t reg_val;

	if (sunxi_gpio_dt_read_alias(&pio, "gpio0") != DRIVER_OK || sunxi_gpio_dt_read_alias(&r_pio, "gpio1") != DRIVER_OK) {
		printk_error("GPIO: invalid PIO devicetree configuration\n");
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

void show_chip()
{
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

	printk_info("Chip SID = %08x%08x%08x%08x\n", chip_sid[0], chip_sid[1], chip_sid[2], chip_sid[3]);

	uint32_t chip_markid_sid = chip_sid[0] & 0xffff;

	switch (chip_markid_sid) {
	case 0x5f00:
		printk_info("Chip type = T536MX-CXX");
		break;
	default:
		printk_info("Chip type = UNKNOW");
		break;
	}

	setbits_le32(SUNXI_SYSCTRL_BASE + 0x24, BIT(15));
	uint32_t version = (read32(SUNXI_SYSCTRL_BASE + 0x24) & 0xFFFF0007) >> 16;
	printk(LOG_LEVEL_MUTE, " Chip Version = 0x%04x \n", version);
}

void sys_reset(void)
{
	write32(SUNXI_WDG_BASE + 0x08, 0x16aa0001U);

	for (;;) {
	}
}
