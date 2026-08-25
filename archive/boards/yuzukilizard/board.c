/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>
#include <io.h>

#include <log.h>

#include <common.h>

#include <drivers/clk/clk.h>
#include <drivers/soc/sid.h>
#include <dt-compatible/sid-dt.h>
#include <dt-bindings/soc/sun8iw21.h>
#include <drivers/clk/sun8iw21/reg.h>

#include <mmu.h>

#include <drivers/spi/spi.h>
#include <drivers/serial/serial.h>
#include <drivers/rtc/rtc.h>

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

void rtc_set_vccio_det_spare(const sunxi_rtc_t *rtc)
{
	uint32_t val = rtc_read_data(rtc, 0x3d);
	val &= ~(0xff << 4);
	val |= (VCCIO_THRESHOLD_VOLTAGE_2_9 | FORCE_DETECTER_OUTPUT);
	val &= ~VCCIO_DET_BYPASS_EN;
	rtc_write_data(rtc, 0x3d, val);
}

void sys_ldo_check(void)
{
	sunxi_sid_t sid;
	uint32_t reg_val = 0;
	uint32_t roughtrim_val = 0, finetrim_val = 0;

	if (sunxi_sid_dt_read_alias(&sid, "sid0") != DRIVER_OK) {
		printk_error("SID: invalid devicetree configuration\n");
		return;
	}

	/* reset */
	reg_val = readl(SUNXI_CCU_BASE + CCU_AUDIO_CODEC_BGR_REG);
	reg_val &= ~(1 << 16);
	writel(reg_val, SUNXI_CCU_BASE + CCU_AUDIO_CODEC_BGR_REG);

	sdelay(2);

	reg_val |= (1 << 16);
	writel(reg_val, SUNXI_CCU_BASE + CCU_AUDIO_CODEC_BGR_REG);

	/* enable AUDIO gating */
	reg_val = readl(SUNXI_CCU_BASE + CCU_AUDIO_CODEC_BGR_REG);
	reg_val |= (1 << 0);
	writel(reg_val, SUNXI_CCU_BASE + CCU_AUDIO_CODEC_BGR_REG);

	/* enable pcrm CTRL */
	reg_val = readl(ANA_PWR_RST_REG);
	reg_val &= ~(1 << 0);
	writel(reg_val, ANA_PWR_RST_REG);

	/* read efuse */
	printk_debug("Audio: avcc calibration\n");
	reg_val = sunxi_efuse_sram_read(&sid, 0x28U);
	roughtrim_val = (reg_val >> 0) & 0xF;
	reg_val = sunxi_efuse_sram_read(&sid, 0x24U);
	finetrim_val = (reg_val >> 16) & 0xFF;

	if (roughtrim_val == 0 && finetrim_val == 0) {
		reg_val = readl(SUNXI_VER_REG);
		reg_val = (reg_val >> 0) & 0x7;
		if (reg_val) {
			printk_debug("Audio: chip not version A\n");
		} else {
			roughtrim_val = 0x5;
			finetrim_val = 0x19;
			printk_debug("Audio: chip version A\n");
		}
	}
	reg_val = readl(AUDIO_POWER_REG);
	reg_val &= ~(0xF << 8 | 0xFF);
	reg_val |= roughtrim_val << 8 | finetrim_val;
	writel(reg_val, AUDIO_POWER_REG);
}

void sys_reset(void)
{
	write32(SUNXI_WDOG_BASE + 0x08, 0x16aa0001U);

	for (;;) {
	}
}
