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
		.base = SUNXI_UART0_BASE,
		.id = 0,
		.baud_rate = UART_BAUDRATE_115200,
		.dlen = UART_DLEN_8,
		.stop = UART_STOP_BIT_0,
		.parity = UART_PARITY_NO,
		.gpio_pin =
				{
						.gpio_tx = {GPIO_PIN(GPIO_PORTB, 8), GPIO_PERIPH_MUX6},
						.gpio_rx = {GPIO_PIN(GPIO_PORTB, 9), GPIO_PERIPH_MUX6},
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
		.base = SUNXI_SPI0_BASE,
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

#if defined(CONFIG_CHIP_MMC_V2)
sunxi_sdhci_t sdhci0 = {
		.name = "sdhci0",
		.id = MMC_CONTROLLER_0,
		.reg_base = SUNXI_SMHC0_BASE,
		.sdhci_mmc_type = MMC_TYPE_SD,
		.max_clk = 50000000,
		.width = SMHC_WIDTH_4BIT,
		.dma_des_addr = SDRAM_BASE + 0x30080000,
		.pinctrl =
				{
						.gpio_clk = {GPIO_PIN(GPIO_PORTF, 2), GPIO_PERIPH_MUX2},
						.gpio_cmd = {GPIO_PIN(GPIO_PORTF, 3), GPIO_PERIPH_MUX2},
						.gpio_d0 = {GPIO_PIN(GPIO_PORTF, 1), GPIO_PERIPH_MUX2},
						.gpio_d1 = {GPIO_PIN(GPIO_PORTF, 0), GPIO_PERIPH_MUX2},
						.gpio_d2 = {GPIO_PIN(GPIO_PORTF, 5), GPIO_PERIPH_MUX2},
						.gpio_d3 = {GPIO_PIN(GPIO_PORTF, 4), GPIO_PERIPH_MUX2},
						.gpio_cd = {GPIO_PIN(GPIO_PORTF, 6), GPIO_INPUT},
						.cd_level = GPIO_LEVEL_LOW,
				},
		.clk_ctrl =
				{
						.gate_reg_base = CCU_BASE + CCU_SMHC_BGR_REG,
						.gate_reg_offset = SDHCI_DEFAULT_CLK_GATE_OFFSET(0),
						.rst_reg_base = CCU_BASE + CCU_SMHC_BGR_REG,
						.rst_reg_offset = SDHCI_DEFAULT_CLK_RST_OFFSET(0),
				},
		.sdhci_clk =
				{
						.reg_base = CCU_BASE + CCU_SMHC0_CLK_REG,
						.reg_factor_n_offset = SDHCI_DEFAULT_CLK_FACTOR_N_OFFSET,
						.reg_factor_m_offset = SDHCI_DEFAULT_CLK_FACTOR_M_OFFSET,
						.clk_sel = 0x1,
						.parent_clk = 300000000,
				},
};

sunxi_sdhci_t sdhci2 = {
		.name = "sdhci2",
		.id = MMC_CONTROLLER_2,
		.reg_base = SUNXI_SMHC2_BASE,
		.sdhci_mmc_type = MMC_TYPE_EMMC,
		.max_clk = 50000000,
		.width = SMHC_WIDTH_4BIT,
		.dma_des_addr = SDRAM_BASE + 0x20080000,
		.pinctrl =
				{
						.gpio_clk = {GPIO_PIN(GPIO_PORTC, 2), GPIO_PERIPH_MUX3},
						.gpio_cmd = {GPIO_PIN(GPIO_PORTC, 3), GPIO_PERIPH_MUX3},
						.gpio_d0 = {GPIO_PIN(GPIO_PORTC, 6), GPIO_PERIPH_MUX3},
						.gpio_d1 = {GPIO_PIN(GPIO_PORTC, 5), GPIO_PERIPH_MUX3},
						.gpio_d2 = {GPIO_PIN(GPIO_PORTC, 4), GPIO_PERIPH_MUX3},
						.gpio_d3 = {GPIO_PIN(GPIO_PORTC, 7), GPIO_PERIPH_MUX3},
				},
		.clk_ctrl =
				{
						.gate_reg_base = CCU_BASE + CCU_SMHC_BGR_REG,
						.gate_reg_offset = SDHCI_DEFAULT_CLK_GATE_OFFSET(2),
						.rst_reg_base = CCU_BASE + CCU_SMHC_BGR_REG,
						.rst_reg_offset = SDHCI_DEFAULT_CLK_RST_OFFSET(2),
				},
		.sdhci_clk =
				{
						.reg_base = CCU_BASE + CCU_SMHC2_CLK_REG,
						.reg_factor_n_offset = SDHCI_DEFAULT_CLK_FACTOR_N_OFFSET,
						.reg_factor_m_offset = SDHCI_DEFAULT_CLK_FACTOR_M_OFFSET,
						.clk_sel = 0x1,
						.parent_clk = 300000000,
				},
};
#else
sdhci_t sdhci0 = {
		.name = "sdhci0",
		.id = 0,
		.reg = (sdhci_reg_t *) 0x04020000,
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
		.id = 0,
		.reg = (sdhci_reg_t *) 0x04022000,
		.voltage = MMC_VDD_27_36,
		.width = MMC_BUS_WIDTH_4,
		.clock = MMC_CLK_50M,
		.removable = 0,
		.isspi = FALSE,
		.skew_auto_mode = TRUE,
		.sdhci_pll = CCU_MMC_CTRL_PLL_PERIPH1X,
		.gpio_clk = {GPIO_PIN(GPIO_PORTC, 2), GPIO_PERIPH_MUX3},
		.gpio_cmd = {GPIO_PIN(GPIO_PORTC, 3), GPIO_PERIPH_MUX3},
		.gpio_d0 = {GPIO_PIN(GPIO_PORTC, 6), GPIO_PERIPH_MUX3},
		.gpio_d1 = {GPIO_PIN(GPIO_PORTC, 5), GPIO_PERIPH_MUX3},
		.gpio_d2 = {GPIO_PIN(GPIO_PORTC, 4), GPIO_PERIPH_MUX3},
		.gpio_d3 = {GPIO_PIN(GPIO_PORTC, 7), GPIO_PERIPH_MUX3},
};
#endif

dram_para_t dram_para = {
		.dram_clk = 720,
		.dram_type = 3,
		.dram_zq = 0x07b7bfb,
		.dram_odt_en = 0x01,
		.dram_para1 = 0x000010d2,
		.dram_para2 = 0,
		.dram_mr0 = 0x1c70,
		.dram_mr1 = 0x42,
		.dram_mr2 = 0x18,
		.dram_mr3 = 0,
		.dram_tpr0 = 0x004A2195,
		.dram_tpr1 = 0x02423190,
		.dram_tpr2 = 0x0008b061,
		.dram_tpr3 = 0xB4787896,// unused
		.dram_tpr4 = 0,
		.dram_tpr5 = 0x48484848,
		.dram_tpr6 = 0x48,
		.dram_tpr7 = 0x1620121e,// unused
		.dram_tpr8 = 0,
		.dram_tpr9 = 0,// clock?
		.dram_tpr10 = 0,
		.dram_tpr11 = 0x00a70000,
		.dram_tpr12 = 0x00100003,
		.dram_tpr13 = 0x3405C100,
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
