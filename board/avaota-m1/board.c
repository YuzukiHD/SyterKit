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

#include <sys-dram.h>
#include <sys-gpio.h>
#include <sys-i2c.h>
#include <sys-sdcard.h>
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
						.gate_reg_base = SUNXI_CCU_BASE + UART0_BGR_REG,
						.gate_reg_offset = SERIAL_DEFAULT_CLK_GATE_OFFSET(0),
						.rst_reg_base = SUNXI_CCU_BASE + UART0_BGR_REG,
						.rst_reg_offset = SERIAL_DEFAULT_CLK_RST_OFFSET(0),
						.parent_clk = SERIAL_DEFAULT_PARENT_CLK,
				},
};

sunxi_i2c_t i2c_pmu = {
		.base = SUNXI_RTWI_BASE,
		.id = SUNXI_R_I2C0,
		.speed = SUNXI_I2C_SPEED_400K,
		.gpio =
				{
						.gpio_scl = {GPIO_PIN(GPIO_PORTL, 0), GPIO_PERIPH_MUX2},
						.gpio_sda = {GPIO_PIN(GPIO_PORTL, 1), GPIO_PERIPH_MUX2},
				},
		.i2c_clk =
				{
						.gate_reg_base = SUNXI_RTWI_BRG_REG,
						.gate_reg_offset = TWI_DEFAULT_CLK_GATE_OFFSET(0),
						.rst_reg_base = SUNXI_RTWI_BRG_REG,
						.rst_reg_offset = TWI_DEFAULT_CLK_RST_OFFSET(0),
						.parent_clk = 24000000,
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
						//.gpio_clk = {GPIO_PIN(GPIO_PORTF, 2), GPIO_PERIPH_MUX2},
						.gpio_cmd = {GPIO_PIN(GPIO_PORTF, 3), GPIO_PERIPH_MUX2},
						.gpio_d0 = {GPIO_PIN(GPIO_PORTF, 1), GPIO_PERIPH_MUX2},
						.gpio_d1 = {GPIO_PIN(GPIO_PORTF, 0), GPIO_PERIPH_MUX2},
						.gpio_d2 = {GPIO_PIN(GPIO_PORTF, 5), GPIO_PERIPH_MUX2},
						//.gpio_d3 = {GPIO_PIN(GPIO_PORTF, 4), GPIO_PERIPH_MUX2},
						.gpio_cd = {GPIO_PIN(GPIO_PORTF, 6), GPIO_INPUT},
						.cd_level = GPIO_LEVEL_LOW,
				},
		.clk_ctrl =
				{
						.gate_reg_base = SUNXI_CCU_BASE + SMHC0_BGR_REG,
						.gate_reg_offset = SDHCI_DEFAULT_CLK_GATE_OFFSET(0),
						.rst_reg_base = SUNXI_CCU_BASE + SMHC0_BGR_REG,
						.rst_reg_offset = SDHCI_DEFAULT_CLK_RST_OFFSET(0),
				},
		.sdhci_clk =
				{
						.reg_base = SUNXI_CCU_BASE + SMHC0_CLK_REG,
						.reg_factor_n_offset = SDHCI_DEFAULT_CLK_FACTOR_N_OFFSET,
						.reg_factor_m_offset = SDHCI_DEFAULT_CLK_FACTOR_M_OFFSET,
						.clk_sel = 0x1,
						.parent_clk = 300000000,
				},
};

uint32_t dram_para[128] = {
		2400,	   // dram_clk
		9,		   // dram_type
		0x0e0e0e0e,// dram_dx_odt
		0x0f0f0f0f,// dram_dx_dri
		0xec030e0f,// dram_ca_dri
		0,		   // dram_para0
		0xa10a,	   // dram_para1
		0x1001,	   // dram_para2
		0,		   // dram_mr0
		0,		   // dram_mr1
		0,		   // dram_mr2
		0x6,	   // dram_mr3
		0,		   // dram_mr4
		0,		   // dram_mr5
		0,		   // dram_mr6
		0x12,	   // dram_mr11
		0x44,	   // dram_mr12
		0,		   // dram_mr13
		0x34,	   // dram_mr14
		0,		   // dram_mr16
		0x06,	   // dram_mr17
		0,		   // dram_mr22
		0x4040,	   // dram_tpr0
		0,		   // dram_tpr1
		0x170b070, // dram_tpr2
		0x3800,	   // dram_tpr3
		0x3514,	   // dram_tpr6
		0x325f0000,// dram_tpr10
		0,		   // dram_tpr11
		0,		   // dram_tpr12
		0x10061,   // dram_tpr13
		0		   // dram_tpr14
};

void neon_enable(void) {
	/* Set the CPACR for access to CP10 and CP11*/
	asm volatile("LDR r0, =0xF00000");
	asm volatile("MCR p15, 0, r0, c1, c0, 2");
	/* Set the FPEXC EN bit to enable the FPU */
	asm volatile("MOV r3, #0x40000000");
	/*@VMSR FPEXC, r3*/
	asm volatile("MCR p10, 7, r3, c8, c0, 0");
}

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
	sunxi_soc_version_t version = sunxi_get_soc_ver();

	if (version == SUNXI_SOC_VER_B)
		writel(0x01155550, SUNXI_PIO_BASE + GPIO_POW_MODE_REG);

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
	uint32_t chip_sid[4];
	chip_sid[0] = read32(SUNXI_SID_SRAM_BASE + 0x0);
	chip_sid[1] = read32(SUNXI_SID_SRAM_BASE + 0x4);
	chip_sid[2] = read32(SUNXI_SID_SRAM_BASE + 0x8);
	chip_sid[3] = read32(SUNXI_SID_SRAM_BASE + 0xc);

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