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
						.gate_reg_base = CCU_BASE + CCU_UART_BGR_REG,
						.gate_reg_offset = SERIAL_DEFAULT_CLK_GATE_OFFSET(0),
						.rst_reg_base = CCU_BASE + CCU_UART_BGR_REG,
						.rst_reg_offset = SERIAL_DEFAULT_CLK_RST_OFFSET(0),
						.parent_clk = SERIAL_DEFAULT_PARENT_CLK,
				},
};

sunxi_sdhci_t sdhci0 = {
		.name = "SD Card",
		.id = MMC_CONTROLLER_0,
		.reg_base = SUNXI_SMHC0_BASE,
		.sdhci_mmc_type = MMC_TYPE_SD,
		.max_clk = 50000000,
		.width = SMHC_WIDTH_4BIT,
		.dma_des_addr = SDRAM_BASE + 0x10080000,
		.pinctrl =
				{
						.gpio_clk = {GPIO_PIN(GPIO_PORTF, 2), GPIO_PERIPH_MUX2},
						.gpio_cmd = {GPIO_PIN(GPIO_PORTF, 3), GPIO_PERIPH_MUX2},
						.gpio_d0 = {GPIO_PIN(GPIO_PORTF, 1), GPIO_PERIPH_MUX2},
						.gpio_d1 = {GPIO_PIN(GPIO_PORTF, 0), GPIO_PERIPH_MUX2},
						.gpio_d2 = {GPIO_PIN(GPIO_PORTF, 5), GPIO_PERIPH_MUX2},
						.gpio_d3 = {GPIO_PIN(GPIO_PORTF, 4), GPIO_PERIPH_MUX2},
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
		.name = "eMMC",
		.id = MMC_CONTROLLER_2,
		.reg_base = SUNXI_SMHC2_BASE,
		.sdhci_mmc_type = MMC_TYPE_EMMC,
		.max_clk = 50000000,
		.width = SMHC_WIDTH_8BIT,
		.dma_des_addr = SDRAM_BASE + 0x10880000,
		.pinctrl =
				{
						.gpio_clk = {GPIO_PIN(GPIO_PORTC, 5), GPIO_PERIPH_MUX3},
						.gpio_cmd = {GPIO_PIN(GPIO_PORTC, 6), GPIO_PERIPH_MUX3},
						.gpio_d0 = {GPIO_PIN(GPIO_PORTC, 10), GPIO_PERIPH_MUX3},
						.gpio_d1 = {GPIO_PIN(GPIO_PORTC, 13), GPIO_PERIPH_MUX3},
						.gpio_d2 = {GPIO_PIN(GPIO_PORTC, 15), GPIO_PERIPH_MUX3},
						.gpio_d3 = {GPIO_PIN(GPIO_PORTC, 8), GPIO_PERIPH_MUX3},
						.gpio_d4 = {GPIO_PIN(GPIO_PORTC, 9), GPIO_PERIPH_MUX3},
						.gpio_d5 = {GPIO_PIN(GPIO_PORTC, 11), GPIO_PERIPH_MUX3},
						.gpio_d6 = {GPIO_PIN(GPIO_PORTC, 14), GPIO_PERIPH_MUX3},
						.gpio_d7 = {GPIO_PIN(GPIO_PORTC, 16), GPIO_PERIPH_MUX3},
						.gpio_ds = {GPIO_PIN(GPIO_PORTC, 0), GPIO_PERIPH_MUX3},
						.gpio_rst = {GPIO_PIN(GPIO_PORTC, 1), GPIO_PERIPH_MUX3},
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
						.parent_clk = 400000000,
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

const uint32_t dram_para[32] = {
		0x318,
		0x8,
		0x7070707,
		0xd0d0d0d,
		0xe0e,
		0xd0a050c,
		0x30fa,
		0x8001000,
		0x0,
		0x34,
		0x1b,
		0x33,
		0x3,
		0x0,
		0x0,
		0x4,
		0x72,
		0x0,
		0x7,
		0x0,
		0x0,
		0x26,
		0x6060606,
		0x4040404,
		0x0,
		0x74000000,
		0x48000000,
		0x273333,
		0x201c181f,
		0x13151513,
		0x7521,
		0x2023211f,
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

#define RTC_DATA_COLD_START (7)
#define CPUS_CODE_LENGTH (0x1000)
#define CPUS_VECTOR_LENGTH (0x4000)

extern uint8_t ar100code_bin[];
extern uint32_t ar100code_bin_len;

int ar100s_gpu_fix(void) {
	uint32_t value;
	uint32_t id = (readl(SUNXI_SYSCRL_BASE + 0x24)) & 0x07;
	printk_debug("SUNXI_SYSCRL_BASE + 0x24 = 0x%08x, id = %d, RTC_DATA_COLD_START = %d\n", readl(SUNXI_SYSCRL_BASE + 0x24), id, rtc_read_data(RTC_DATA_COLD_START));
	if (((id == 0) || (id == 3) || (id == 4) || (id == 5))) {
		if (rtc_read_data(RTC_DATA_COLD_START) == 0) {
			rtc_write_data(RTC_DATA_COLD_START, 0x1);

			value = readl(SUNXI_RCPUCFG_BASE + 0x0);
			value &= ~1;
			writel(value, SUNXI_RCPUCFG_BASE + 0x0);

			memcpy((void *) SCP_SRAM_BASE, (void *) ar100code_bin, CPUS_CODE_LENGTH);
			memcpy((void *) (SCP_SRAM_BASE + CPUS_VECTOR_LENGTH), (char *) ar100code_bin + CPUS_CODE_LENGTH, ar100code_bin_len - CPUS_CODE_LENGTH);
			asm volatile("dsb");

			value = readl(SUNXI_RCPUCFG_BASE + 0x0);
			value &= ~1;
			writel(value, SUNXI_RCPUCFG_BASE + 0x0);
			value = readl(SUNXI_RCPUCFG_BASE + 0x0);
			value |= 1;
			writel(value, SUNXI_RCPUCFG_BASE + 0x0);
			while (1) asm volatile("WFI");
		} else {
			rtc_write_data(RTC_DATA_COLD_START, 0x0);
		}
	}

	return 0;
}

void show_chip() {
	uint32_t chip_sid[4];
	chip_sid[0] = read32(SUNXI_SID_SRAM_BASE + 0x0);
	chip_sid[1] = read32(SUNXI_SID_SRAM_BASE + 0x4);
	chip_sid[2] = read32(SUNXI_SID_SRAM_BASE + 0x8);
	chip_sid[3] = read32(SUNXI_SID_SRAM_BASE + 0xc);

	printk_info("Model: mCore-R818 Core Board.\n");
	printk_info("Core: Quad-Core Cortex-A53\n");
	printk_info("Chip SID = %08x%08x%08x%08x\n", chip_sid[0], chip_sid[1], chip_sid[2], chip_sid[3]);

	uint32_t chip_markid_sid = chip_sid[0] & 0xffff;

	switch (chip_markid_sid) {
		case 0x1000:
			printk_info("Chip type = R818\n");
			break;
		default:
			printk_info("Chip type = UNKNOW\n");
			break;
	}
}
