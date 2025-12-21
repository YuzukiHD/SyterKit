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
#include <mmc/sys-sdcard.h>

#include <sys-dram.h>
#include <sys-gpio.h>
#include <sys-i2c.h>
#include <sys-sid.h>
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
						.gpio_tx = {GPIO_PIN(GPIO_PORTB, 9), GPIO_PERIPH_MUX2},
						.gpio_rx = {GPIO_PIN(GPIO_PORTB, 10), GPIO_PERIPH_MUX2},
				},
		.uart_clk =
				{
						.gate_reg_base = SUNXI_CCM_BASE + UART0_GAR_REG,
						.gate_reg_offset = SERIAL_DEFAULT_CLK_GATE_OFFSET(0),
						.rst_reg_base = SUNXI_CCM_BASE + UART0_GAR_REG,
						.rst_reg_offset = SERIAL_DEFAULT_CLK_RST_OFFSET(0),
						.parent_clk = SERIAL_DEFAULT_PARENT_CLK,
				},
};

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
						.gate_reg_base = SUNXI_CCM_BASE + SMHC0_GAR_REG,
						.gate_reg_offset = SDHCI_DEFAULT_CLK_GATE_OFFSET(0),
						.rst_reg_base = SUNXI_CCM_BASE + SMHC0_GAR_REG,
						.rst_reg_offset = SDHCI_DEFAULT_CLK_RST_OFFSET(0),
				},
		.sdhci_clk =
				{
						.reg_base = SUNXI_CCM_BASE + SMHC0_CLK_REG,
						.reg_factor_n_offset = SDHCI_DEFAULT_CLK_FACTOR_N_OFFSET,
						.reg_factor_m_offset = SDHCI_DEFAULT_CLK_FACTOR_M_OFFSET,
						.clk_sel = 0x1,
						.parent_clk = 400000000,
				},
};

sunxi_dma_t sunxi_dma = {
		.dma_reg_base = SUNXI_DMA_BASE,
		.bus_clk =
				{
						.gate_reg_base = SUNXI_CCM_BASE + MBUS_CLK_GATE_EN_REG,
						.gate_reg_offset = DMA_DEFAULT_CLK_GATE_OFFSET,
				},
		.dma_clk =
				{
						.rst_reg_base = SUNXI_CCM_BASE + DMA0_GAR_REG,
						.rst_reg_offset = DMA_DEFAULT_CLK_RST_OFFSET,
						.gate_reg_base = SUNXI_CCM_BASE + DMA0_GAR_REG,
						.gate_reg_offset = DMA_DEFAULT_CLK_GATE_OFFSET,
				},
};

sunxi_spi_t sunxi_spi0 = {
		.base = SUNXI_SPI0_BASE,
		.id = 0,
		.clk_rate = 100 * 1000 * 1000,
		.gpio =
				{
						.gpio_cs = {GPIO_PIN(GPIO_PORTC, 1), GPIO_PERIPH_MUX3},
						.gpio_sck = {GPIO_PIN(GPIO_PORTC, 0), GPIO_PERIPH_MUX3},
						.gpio_mosi = {GPIO_PIN(GPIO_PORTC, 2), GPIO_PERIPH_MUX3},
						.gpio_miso = {GPIO_PIN(GPIO_PORTC, 3), GPIO_PERIPH_MUX3},
						.gpio_wp = {GPIO_PIN(GPIO_PORTC, 4), GPIO_PERIPH_MUX3},
						.gpio_hold = {GPIO_PIN(GPIO_PORTC, 5), GPIO_PERIPH_MUX3},
				},
		.spi_clk =
				{
						.spi_clock_cfg_base = SUNXI_CCM_BASE + SPI0_CLK_REG,
						.spi_clock_factor_n_offset = SPI_CLK_SEL_FACTOR_N_OFF,
						.spi_clock_source = SPI0_CLK_REG_CLK_SRC_SEL_PERI0_300M,
				},
		.parent_clk_reg =
				{
						.rst_reg_base = SUNXI_CCM_BASE + SPI0_GAR_REG,
						.rst_reg_offset = SPI_DEFAULT_CLK_RST_OFFSET(0),
						.gate_reg_base = SUNXI_CCM_BASE + SPI0_GAR_REG,
						.gate_reg_offset = SPI_DEFAULT_CLK_GATE_OFFSET(0),
						.parent_clk = 300000000,
				},
		.dma_handle = &sunxi_dma,
};

uint32_t dram_para[96] = {
		1200,	   // dram_clk
		4,		   // dram_type
		0x00000808,// dram_dx_odt
		0x00000c0c,// dram_dx_dri
		0x0c0c0c,  // dram_ca_dri
		0x00006060,// dram_para0
		0x60fa,	   // dram_para1
		0x0001,	   // dram_para2
		0x520,	   // dram_mr0
		0x101,	   // dram_mr1
		0x8,	   // dram_mr2
		0x0,	   // dram_mr3
		0x0,	   // dram_mr4
		0x400,	   // dram_mr5
		0x81a,	   // dram_mr6
		0x0,	   // dram_mr11
		0x0,	   // dram_mr12
		0x0,	   // dram_mr13
		0x0,	   // dram_mr14
		0x0,	   // dram_mr16
		0x0,	   // dram_mr17
		0x0,	   // dram_mr22
		0x0,	   // dram_tpr0
		0x0,	   // dram_tpr1
		0x0,	   // dram_tpr2
		0x0,	   // dram_tpr3
		0x4000,	   // dram_tpr6
		0x00002500,// dram_tpr10
		0x00005050,// dram_tpr11
		0x00002020,// dram_tpr12
		0x00001070,// dram_tpr13
		0x810700f5,// dram_tpr14
};

uint32_t dram_para_trained[96] = {
		0x000004b0,// dram_clk
		0x00000004,// dram_type
		0x00000808,// dram_dx_odt
		0x00000c0c,// dram_dx_dri
		0x000c0c0c,// dram_ca_dri
		0x00006060,// dram_para0
		0x000060fa,// dram_para1
		0x02000001,// dram_para2
		0x00000964,// dram_mr0
		0x00000101,// dram_mr1
		0x00000018,// dram_mr2
		0x00000000,// dram_mr3
		0x00000000,// dram_mr4
		0x00000400,// dram_mr5
		0x00000813,// dram_mr6
		0x00000000,// dram_mr11
		0x00000000,// dram_mr12
		0x00000000,// dram_mr13
		0x00000000,// dram_mr14
		0x00000000,// dram_mr16
		0x00000000,// dram_mr17
		0x00000000,// dram_mr22
		0x00000000,// dram_tpr0
		0x00000000,// dram_tpr1
		0x00000000,// dram_tpr2
		0x00000000,// dram_tpr3
		0x00004000,// dram_tpr6
		0x00002500,// dram_tpr10
		0x00005555,// dram_tpr11
		0x0000201f,// dram_tpr12
		0x00007071,// dram_tpr13
		0x010700f5,// dram_tpr14
};

void neon_enable(void) {
	/* set NSACR, both Secure and Non-secure access are allowed to NEON */
	asm volatile("MRC p15, 0, r0, c1, c1, 2");
	asm volatile("ORR r0, r0, #(0x3<<10) @ enable fpu/neon");
	asm volatile("MCR p15, 0, r0, c1, c1, 2");
	/* Set the CPACR for access to CP10 and CP11*/
	asm volatile("LDR r0, =0xF00000");
	asm volatile("MCR p15, 0, r0, c1, c0, 2");
	/* Set the FPEXC EN bit to enable the FPU */
	asm volatile("MOV r3, #0x40000000");
	/*@VMSR FPEXC, r3*/
	asm volatile("MCR p10, 7, r3, c8, c0, 0");
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
	uint32_t chip_sid[4];
	chip_sid[0] = read32(SUNXI_SID_SRAM_BASE + 0x0);
	chip_sid[1] = read32(SUNXI_SID_SRAM_BASE + 0x4);
	chip_sid[2] = read32(SUNXI_SID_SRAM_BASE + 0x8);
	chip_sid[3] = read32(SUNXI_SID_SRAM_BASE + 0xc);

	printk_info("Chip SID = %08x%08x%08x%08x\n", chip_sid[0], chip_sid[1], chip_sid[2], chip_sid[3]);

	uint32_t chip_markid_sid = chip_sid[0] & 0xffff;

	switch (chip_markid_sid) {
		case 0x7700:
			printk_info("Chip type = T153MX-BCX");
			break;
		default:
			printk_info("Chip type = UNKNOW");
			break;
	}

	setbits_le32(SUNXI_SYSCTRL_BASE + 0x24, BIT(15));
	uint32_t version = (read32(SUNXI_SYSCTRL_BASE + 0x24) & 0xFFFF0007) >> 16;
	printk(LOG_LEVEL_MUTE, " Chip Version = 0x%04x \n", version);
}