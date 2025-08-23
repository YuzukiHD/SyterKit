/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>

#include <common.h>

#include <reg-ncat.h>
#include <sys-clk.h>

#include <mmu.h>

#include <sys-dram.h>
#include <sys-gpio.h>
#include <sys-sdcard.h>
#include <sys-spi.h>
#include <sys-uart.h>

sunxi_serial_t uart_dbg = {
		.base = 0x02500000,
		.id = 0,
		.baud_rate = UART_BAUDRATE_115200,
		.dlen = UART_DLEN_8,
		.stop = UART_STOP_BIT_0,
		.parity = UART_PARITY_NO,
		.gpio_pin =
				{
						.gpio_tx = {GPIO_PIN(GPIO_PORTH, 9), GPIO_PERIPH_MUX5},
						.gpio_rx = {GPIO_PIN(GPIO_PORTH, 10), GPIO_PERIPH_MUX5},
				},
		.uart_clk =
				{
						.gate_reg_base = CCU_BASE + CCU_UART_BGR_REG,
						.gate_reg_offset = SERIAL_DEFAULT_CLK_GATE_OFFSET(0),
						.rst_reg_base = CCU_BASE + CCU_UART_BGR_REG,
						.rst_reg_offset = SERIAL_DEFAULT_CLK_RST_OFFSET(0),
						.parent_clk = SERIAL_DEFAULT_PARENT_CLK,
				},
};

sunxi_dma_t sunxi_dma = {
		.dma_reg_base = SUNXI_DMA_BASE,
		.bus_clk =
				{
						.gate_reg_base = CCU_BASE + CCU_MBUS_MAT_CLK_GATING_REG,
						.gate_reg_offset = DMA_DEFAULT_CLK_GATE_OFFSET,
				},
		.dma_clk =
				{
						.rst_reg_base = CCU_BASE + CCU_DMA_BGR_REG,
						.rst_reg_offset = DMA_DEFAULT_CLK_RST_OFFSET,
						.gate_reg_base = CCU_BASE + CCU_DMA_BGR_REG,
						.gate_reg_offset = DMA_DEFAULT_CLK_GATE_OFFSET,
				},
};

sunxi_spi_t sunxi_spi0 = {
		.base = 0x04025000,
		.id = 0,
		.clk_rate = 75 * 1000 * 1000,
		.gpio =
				{
						.gpio_cs = {GPIO_PIN(GPIO_PORTC, 1), GPIO_PERIPH_MUX4},
						.gpio_sck = {GPIO_PIN(GPIO_PORTC, 0), GPIO_PERIPH_MUX4},
						.gpio_mosi = {GPIO_PIN(GPIO_PORTC, 2), GPIO_PERIPH_MUX4},
						.gpio_miso = {GPIO_PIN(GPIO_PORTC, 3), GPIO_PERIPH_MUX4},
						.gpio_wp = {GPIO_PIN(GPIO_PORTC, 4), GPIO_PERIPH_MUX4},
						.gpio_hold = {GPIO_PIN(GPIO_PORTC, 5), GPIO_PERIPH_MUX4},
				},
		.spi_clk =
				{
						.spi_clock_cfg_base = CCU_BASE + CCU_SPI0_CLK_REG,
						.spi_clock_factor_n_offset = SPI_CLK_SEL_FACTOR_N_OFF,
						.spi_clock_source = SPI_CLK_SEL_PERIPH_300M,
				},
		.parent_clk_reg =
				{
						.rst_reg_base = CCU_BASE + CCU_SPI_BGR_REG,
						.rst_reg_offset = SPI_DEFAULT_CLK_RST_OFFSET(0),
						.gate_reg_base = CCU_BASE + CCU_SPI_BGR_REG,
						.gate_reg_offset = SPI_DEFAULT_CLK_GATE_OFFSET(0),
						.parent_clk = 300000000,
				},
		.dma_handle = &sunxi_dma,
};

sdhci_t sdhci0 = {
		.name = "sdhci0",
		.id = 0,
		.reg = (sdhci_reg_t *) SUNXI_SMHC0_BASE,
		.voltage = MMC_VDD_27_36,
		.width = MMC_BUS_WIDTH_4,
		.clock = MMC_CLK_50M,
		.removable = 0,
		.isspi = FALSE,
		.skew_auto_mode = TRUE,
		.sdhci_pll = CCU_MMC_CTRL_PLL_PERIPH1X,
		.gpio_clk = {GPIO_PIN(GPIO_PORTF, 2), GPIO_PERIPH_MUX2},
		.gpio_cmd = {GPIO_PIN(GPIO_PORTF, 3), GPIO_PERIPH_MUX2},
		.gpio_d0 = {GPIO_PIN(GPIO_PORTF, 1), GPIO_PERIPH_MUX2},
		.gpio_d1 = {GPIO_PIN(GPIO_PORTF, 0), GPIO_PERIPH_MUX2},
		.gpio_d2 = {GPIO_PIN(GPIO_PORTF, 5), GPIO_PERIPH_MUX2},
		.gpio_d3 = {GPIO_PIN(GPIO_PORTF, 4), GPIO_PERIPH_MUX2},
};

sdhci_t sdhci2 = {
		.name = "sdhci2",
		.id = 2,
		.reg = (sdhci_reg_t *) SUNXI_SMHC2_BASE,
		.voltage = MMC_VDD_27_36,
		.width = MMC_BUS_WIDTH_4,
		.clock = MMC_CLK_50M,
		.removable = 0,
		.isspi = FALSE,
		.skew_auto_mode = TRUE,
		.sdhci_pll = CCU_MMC_CTRL_PLL_PERIPH1X,
		.gpio_clk = {GPIO_PIN(GPIO_PORTC, 0), GPIO_PERIPH_MUX3},
		.gpio_cmd = {GPIO_PIN(GPIO_PORTC, 1), GPIO_PERIPH_MUX3},
		.gpio_d0 = {GPIO_PIN(GPIO_PORTC, 4), GPIO_PERIPH_MUX3},
		.gpio_d1 = {GPIO_PIN(GPIO_PORTC, 3), GPIO_PERIPH_MUX3},
		.gpio_d2 = {GPIO_PIN(GPIO_PORTC, 2), GPIO_PERIPH_MUX3},
		.gpio_d3 = {GPIO_PIN(GPIO_PORTC, 5), GPIO_PERIPH_MUX3},
};

dram_para_t dram_para = {
		.dram_clk = 528,
		.dram_type = 2,
		.dram_zq = 0x7b7bf9,
		.dram_odt_en = 0x0,
		.dram_para1 = 0x00d2,
		.dram_para2 = 0x0,
		.dram_mr0 = 0xe73,
		.dram_mr1 = 0x02,
		.dram_mr2 = 0x0,
		.dram_mr3 = 0x0,
		.dram_tpr0 = 0x00471992,
		.dram_tpr1 = 0x0131a10c,
		.dram_tpr2 = 0x00057041,
		.dram_tpr3 = 0xb4787896,
		.dram_tpr4 = 0x0,
		.dram_tpr5 = 0x48484848,
		.dram_tpr6 = 0x48,
		.dram_tpr7 = 0x1621121e,
		.dram_tpr8 = 0x0,
		.dram_tpr9 = 0x0,
		.dram_tpr10 = 0x00000000,
		.dram_tpr11 = 0x00000022,
		.dram_tpr12 = 0x00000077,
		.dram_tpr13 = 0x34000100,
};

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

void rtc_set_vccio_det_spare(void) {
	u32 val = 0;
	val = readl(SUNXI_RTC_BASE + 0x1f4);
	val &= ~(0xff << 4);
	val |= (VCCIO_THRESHOLD_VOLTAGE_2_9 | FORCE_DETECTER_OUTPUT);
	val &= ~VCCIO_DET_BYPASS_EN;
	writel(val, SUNXI_RTC_BASE + 0x1f4);
}

void sys_ldo_check(void) {
	uint32_t reg_val = 0;
	uint32_t roughtrim_val = 0, finetrim_val = 0;

	/* reset */
	reg_val = readl(CCU_AUDIO_CODEC_BGR_REG);
	reg_val &= ~(1 << 16);
	writel(reg_val, CCU_AUDIO_CODEC_BGR_REG);

	sdelay(2);

	reg_val |= (1 << 16);
	writel(reg_val, CCU_AUDIO_CODEC_BGR_REG);

	/* enable AUDIO gating */
	reg_val = readl(CCU_AUDIO_CODEC_BGR_REG);
	reg_val |= (1 << 0);
	writel(reg_val, CCU_AUDIO_CODEC_BGR_REG);

	/* enable pcrm CTRL */
	reg_val = readl(ANA_PWR_RST_REG);
	reg_val &= ~(1 << 0);
	writel(reg_val, ANA_PWR_RST_REG);

	/* read efuse */
	printk_debug("Audio: avcc calibration\n");
	reg_val = readl(SUNXI_SID_SRAM_BASE + 0x28);
	roughtrim_val = (reg_val >> 0) & 0xF;
	reg_val = readl(SUNXI_SID_SRAM_BASE + 0x24);
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