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

#include <mmc/sys-sdhci.h>
#include <sys-dma.h>
#include <sys-dram.h>
#include <sys-gpio.h>
#include <sys-i2c.h>
#include <sys-pwm.h>
#include <sys-spi.h>
#include <sys-uart.h>

#include <e907/sysmap.h>

sunxi_serial_t uart_dbg = {
		.base = SUNXI_UART0_BASE,
		.id = 0,
		.baud_rate = UART_BAUDRATE_115200,
		.dlen = UART_DLEN_8,
		.stop = UART_STOP_BIT_0,
		.parity = UART_PARITY_NO,
		.gpio_pin =
				{
						.gpio_tx = {GPIO_PIN(GPIO_PORTL, 4), GPIO_PERIPH_MUX3},
						.gpio_rx = {GPIO_PIN(GPIO_PORTL, 5), GPIO_PERIPH_MUX3},
				},
		.uart_clk =
				{
						.gate_reg_base = SUNXI_CCU_APP_BASE + BUS_CLK_GATING0_REG,
						.gate_reg_offset = BUS_CLK_GATING0_REG_UART0_PCLK_EN_OFFSET,
						.rst_reg_base = SUNXI_CCU_APP_BASE + BUS_Reset0_REG,
						.rst_reg_offset = BUS_Reset0_REG_PRESETN_UART0_SW_OFFSET,
						.parent_clk = 192000000,
				},
};

sunxi_serial_t uart_card = {
		.base = SUNXI_UART0_BASE,
		.id = 0,
		.baud_rate = UART_BAUDRATE_115200,
		.dlen = UART_DLEN_8,
		.stop = UART_STOP_BIT_0,
		.parity = UART_PARITY_NO,
		.gpio_pin =
				{
						.gpio_tx = {GPIO_PIN(GPIO_PORTC, 2), GPIO_PERIPH_MUX3},
						.gpio_rx = {GPIO_PIN(GPIO_PORTC, 4), GPIO_PERIPH_MUX3},
				},
		.uart_clk =
				{
						.gate_reg_base = SUNXI_CCU_APP_BASE + BUS_CLK_GATING0_REG,
						.gate_reg_offset = BUS_CLK_GATING0_REG_UART0_PCLK_EN_OFFSET,
						.rst_reg_base = SUNXI_CCU_APP_BASE + BUS_Reset0_REG,
						.rst_reg_offset = BUS_Reset0_REG_PRESETN_UART0_SW_OFFSET,
						.parent_clk = 192000000,
				},
};

sunxi_dma_t sunxi_dma = {
		.dma_reg_base = SUNXI_DMA_BASE,
		.bus_clk =
				{
						.gate_reg_base = SUNXI_CCU_APP_BASE + BUS_CLK_GATING2_REG,
						.gate_reg_offset = BUS_CLK_GATING2_REG_SGDMA_MCLK_EN_OFFSET,
				},
		.dma_clk =
				{
						.rst_reg_base = SUNXI_CCU_APP_BASE + BUS_Reset0_REG,
						.rst_reg_offset = BUS_Reset0_REG_HRESETN_SGDMA_SW_OFFSET,
						.gate_reg_base = SUNXI_CCU_APP_BASE + BUS_CLK_GATING0_REG,
						.gate_reg_offset = BUS_CLK_GATING0_REG_SGDMA_HCLK_EN_OFFSET,
				},
};

sunxi_spi_t sunxi_spi0 = {
		.base = SUNXI_SPI0_BASE,
		.id = 0,
		.clk_rate = 100 * 1000 * 1000,
		.gpio =
				{
						.gpio_cs = {GPIO_PIN(GPIO_PORTC, 10), GPIO_PERIPH_MUX3},
						.gpio_sck = {GPIO_PIN(GPIO_PORTC, 9), GPIO_PERIPH_MUX3},
						.gpio_mosi = {GPIO_PIN(GPIO_PORTC, 8), GPIO_PERIPH_MUX3},
						.gpio_miso = {GPIO_PIN(GPIO_PORTC, 11), GPIO_PERIPH_MUX3},
						.gpio_wp = {GPIO_PIN(GPIO_PORTC, 6), GPIO_PERIPH_MUX3},
						.gpio_hold = {GPIO_PIN(GPIO_PORTC, 7), GPIO_PERIPH_MUX3},
				},
		.spi_clk =
				{
						.spi_clock_cfg_base = SUNXI_CCU_APP_BASE + SPI_CLK_REG,
						.spi_clock_factor_n_offset = SPI_CLK_REG_SPI_SCLK_DIV2_OFFSET,
						.spi_clock_source = SPI_CLK_REG_SPI_SCLK_SEL_PERI_307M,
						.cdr_mode = SPI_CDR_NONE,
				},
		.parent_clk_reg =
				{
						.rst_reg_base = SUNXI_CCU_APP_BASE + BUS_Reset1_REG,
						.rst_reg_offset = BUS_Reset1_REG_HRESETN_SPI_SW_OFFSET,
						.gate_reg_base = SUNXI_CCU_APP_BASE + BUS_CLK_GATING1_REG,
						.gate_reg_offset = BUS_CLK_GATING1_REG_SPI_HCLK_EN_OFFSET,
						.parent_clk = 307200000,
				},
		.dma_handle = &sunxi_dma,
};

sunxi_i2c_t sunxi_i2c0 = {
		.base = SUNXI_TWI0_BASE,
		.id = SUNXI_I2C0,
		.speed = SUNXI_I2C_SPEED_400K,
		.gpio =
				{
						.gpio_scl = {GPIO_PIN(GPIO_PORTA, 3), GPIO_PERIPH_MUX4},
						.gpio_sda = {GPIO_PIN(GPIO_PORTA, 4), GPIO_PERIPH_MUX4},
				},
		.i2c_clk =
				{
						.gate_reg_base = SUNXI_CCU_APP_BASE + BUS_CLK_GATING0_REG,
						.gate_reg_offset = BUS_CLK_GATING0_REG_TWI0_PCLK_EN_OFFSET,
						.rst_reg_base = SUNXI_CCU_APP_BASE + BUS_Reset0_REG,
						.rst_reg_offset = BUS_Reset0_REG_PRESETN_TWI0_SW_OFFSET,
						.parent_clk = 192000000,
				},
};

sunxi_sdhci_t sdhci0 = {
		.name = "sdhci0",
		.id = MMC_CONTROLLER_0,
		.reg_base = SUNXI_SMHC0_BASE,
		.sdhci_mmc_type = MMC_TYPE_SD,
		.max_clk = 50000000,
		.width = SMHC_WIDTH_4BIT,
		.dma_des_addr = SDRAM_BASE + 0x80000,
		.pinctrl =
				{
						.gpio_clk = {GPIO_PIN(GPIO_PORTC, 2), GPIO_PERIPH_MUX2},
						.gpio_cmd = {GPIO_PIN(GPIO_PORTC, 3), GPIO_PERIPH_MUX2},
						.gpio_d0 = {GPIO_PIN(GPIO_PORTC, 1), GPIO_PERIPH_MUX2},
						.gpio_d1 = {GPIO_PIN(GPIO_PORTC, 0), GPIO_PERIPH_MUX2},
						.gpio_d2 = {GPIO_PIN(GPIO_PORTC, 5), GPIO_PERIPH_MUX2},
						.gpio_d3 = {GPIO_PIN(GPIO_PORTC, 4), GPIO_PERIPH_MUX2},
				},
		.clk_ctrl =
				{
						.gate_reg_base = SUNXI_CCU_APP_BASE + BUS_CLK_GATING1_REG,
						.gate_reg_offset = BUS_CLK_GATING1_REG_SMHC0_HCLK_EN_OFFSET,
						.rst_reg_base = SUNXI_CCU_APP_BASE + BUS_Reset1_REG,
						.rst_reg_offset = BUS_Reset1_REG_HRESETN_SMHC0_SW_OFFSET,
				},
		.sdhci_clk =
				{
						.reg_base = SUNXI_CCU_APP_BASE + SMHC_CTRL0_CLK_REG,
						.reg_factor_n_offset = SMHC_CTRL0_CLK_REG_SMHC_CTRL0_CLK_DIV2_OFFSET,
						.reg_factor_m_offset = SMHC_CTRL0_CLK_REG_SMHC_CTRL0_CLK_DIV1_OFFSET,
						.clk_sel = SMHC_CTRL0_CLK_REG_SMHC_CTRL0_CLK_SEL_PERI_192M,
						.parent_clk = 192000000,
				},
};

sunxi_sdhci_t sdhci1 = {
		.name = "sdhci1",
		.id = MMC_CONTROLLER_1,
		.reg_base = SUNXI_SMHC1_BASE,
		.sdhci_mmc_type = MMC_TYPE_SD,
		.max_clk = 50000000,
		.width = SMHC_WIDTH_4BIT,
		.dma_des_addr = SDRAM_BASE + 0x80000,
		.pinctrl =
				{
						.gpio_clk = {GPIO_PIN(GPIO_PORTC, 2), GPIO_PERIPH_MUX2},
						.gpio_cmd = {GPIO_PIN(GPIO_PORTC, 3), GPIO_PERIPH_MUX2},
						.gpio_d0 = {GPIO_PIN(GPIO_PORTC, 1), GPIO_PERIPH_MUX2},
						.gpio_d1 = {GPIO_PIN(GPIO_PORTC, 0), GPIO_PERIPH_MUX2},
						.gpio_d2 = {GPIO_PIN(GPIO_PORTC, 5), GPIO_PERIPH_MUX2},
						.gpio_d3 = {GPIO_PIN(GPIO_PORTC, 4), GPIO_PERIPH_MUX2},
				},
		.clk_ctrl =
				{
						.gate_reg_base = SUNXI_CCU_APP_BASE + BUS_CLK_GATING1_REG,
						.gate_reg_offset = BUS_CLK_GATING1_REG_SMHC1_HCLK_EN_OFFSET,
						.rst_reg_base = SUNXI_CCU_APP_BASE + BUS_Reset1_REG,
						.rst_reg_offset = BUS_Reset1_REG_HRESETN_SMHC1_SW_OFFSET,
				},
		.sdhci_clk =
				{
						.reg_base = SUNXI_CCU_APP_BASE + SMHC_CTRL1_CLK_REG,
						.reg_factor_n_offset = SMHC_CTRL1_CLK_REG_SMHC_CTRL1_CLK_DIV2_OFFSET,
						.reg_factor_m_offset = SMHC_CTRL1_CLK_REG_SMHC_CTRL1_CLK_DIV1_OFFSET,
						.clk_sel = SMHC_CTRL1_CLK_REG_SMHC_CTRL1_CLK_SEL_PERI_192M,
						.parent_clk = 192000000,
				},
};

sunxi_pwm_channel_t pwm_channel[] = {
		{
				.pin = {GPIO_PIN(GPIO_PORTD, 1), GPIO_PERIPH_MUX5},
				.channel_mode = PWM_CHANNEL_SINGLE,
		},
		{
				.pin = {GPIO_PIN(GPIO_PORTD, 2), GPIO_PERIPH_MUX5},
				.channel_mode = PWM_CHANNEL_SINGLE,
		},
		{
				.pin = {GPIO_PIN(GPIO_PORTD, 3), GPIO_PERIPH_MUX5},
				.bind_channel = 3,
				.dead_time = 4000,
				.channel_mode = PWM_CHANNEL_BIND,
		},
		{
				.pin = {GPIO_PIN(GPIO_PORTD, 4), GPIO_PERIPH_MUX5},
				.bind_channel = 2,
				.dead_time = 4000,
				.channel_mode = PWM_CHANNEL_BIND,
		},
};

sunxi_pwm_t pwm_chip0 = {
		.base = SUNXI_PWM_BASE,
		.id = 0,
		.channel = pwm_channel,
		.channel_size = 4,
		.pwm_clk =
				{
						.gate_reg_base = SUNXI_CCU_APP_BASE + BUS_CLK_GATING0_REG,
						.gate_reg_offset = BUS_CLK_GATING0_REG_PWM_PCLK_EN_OFFSET,
						.rst_reg_base = SUNXI_CCU_APP_BASE + BUS_Reset0_REG,
						.rst_reg_offset = BUS_Reset0_REG_PRESETN_PWM_SW_OFFSET,
				},
		.clk_src =
				{
						.clk_src_hosc = 40000000,
						.clk_src_apb = 384000000,
				},
};

dram_para_t dram_para = {
		.dram_clk = 528,
		.dram_type = 2,
		.dram_zq = 0x7b7bf9,
		.dram_odt_en = 0x00,
		.dram_para1 = 0x000000d2,
		.dram_para2 = 0x00400000,
		.dram_mr0 = 0x00000E73,
		.dram_mr1 = 0x02,
		.dram_mr2 = 0x0,
		.dram_mr3 = 0x0,
		.dram_tpr0 = 0x00471992,
		.dram_tpr1 = 0x0131A10C,
		.dram_tpr2 = 0x00057041,
		.dram_tpr3 = 0xB4787896,
		.dram_tpr4 = 0x0,
		.dram_tpr5 = 0x48484848,
		.dram_tpr6 = 0x48,
		.dram_tpr7 = 0x1621121e,
		.dram_tpr8 = 0x0,
		.dram_tpr9 = 0x0,
		.dram_tpr10 = 0x00000000,
		.dram_tpr11 = 0x00000000,
		.dram_tpr12 = 0x00000000,
		.dram_tpr13 = 0x34000100,
};

void show_chip() {
	uint32_t chip_sid[4];
	chip_sid[0] = read32(SUNXI_SID_SRAM_BASE + 0x0);
	chip_sid[1] = read32(SUNXI_SID_SRAM_BASE + 0x4);
	chip_sid[2] = read32(SUNXI_SID_SRAM_BASE + 0x8);
	chip_sid[3] = read32(SUNXI_SID_SRAM_BASE + 0xc);

	printk_info("Model: AvaotaSBC Avaota F1 board.\n");
	printk_info("Core: XuanTie E907 RISC-V Core.\n");
	printk_info("Chip SID = %08x%08x%08x%08x\n", chip_sid[0], chip_sid[1], chip_sid[2], chip_sid[3]);
}

extern uint8_t current_hosc_freq;
int sunxi_hosc_detect(void) {
	uint32_t val = readl(CCU_HOSC_FREQ_DET_REG);

	writel(val & (~HOSC_FREQ_DET_HOSC_CLEAR_MASK), CCU_HOSC_FREQ_DET_REG);
	writel(val | HOSC_FREQ_DET_HOSC_ENABLE_DETECT, CCU_HOSC_FREQ_DET_REG);

	while (!(HOSC_FREQ_DET_HOSC_FREQ_READY_CLEAR_MASK & readl(CCU_HOSC_FREQ_DET_REG)))
		;

	val = (readl(CCU_HOSC_FREQ_DET_REG) & HOSC_FREQ_DET_HOSC_FREQ_DET_CLEAR_MASK) >> HOSC_FREQ_DET_HOSC_FREQ_DET_OFFSET;
	if (val < ((HOSC_24M_COUNTER + HOSC_40M_COUNTER) / 2)) {
		current_hosc_freq = HOSC_FREQ_24M;
		return HOSC_FREQ_24M;
	} else {
		current_hosc_freq = HOSC_FREQ_40M;
		return HOSC_FREQ_40M;
	}
}

void sysmap_init(void) {
	sysmap_add_mem_region(0x00000000, 0x10000000, SYSMAP_MEM_ATTR_RAM);
	sysmap_add_mem_region(0x10000000, 0x02000000, SYSMAP_MEM_ATTR_RAM);
	sysmap_add_mem_region(0x12000000, 0x1E000000, SYSMAP_MEM_ATTR_DEVICE);
	sysmap_add_mem_region(0x30000000, 0x10000000, SYSMAP_MEM_ATTR_DEVICE);
	sysmap_add_mem_region(0x40000000, 0x28000000, SYSMAP_MEM_ATTR_DEVICE);
	sysmap_add_mem_region(0x68000000, 0x01000000, SYSMAP_MEM_ATTR_DEVICE);
	sysmap_add_mem_region(0x69000000, 0x17000000, SYSMAP_MEM_ATTR_DEVICE);
	sysmap_add_mem_region(0x80000000, 0x7FFFFFFF, SYSMAP_MEM_ATTR_RAM);
}
