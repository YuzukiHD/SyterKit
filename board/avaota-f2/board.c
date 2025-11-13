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

sunxi_serial_t uart_dbg_ph1 = {
		.base = SUNXI_UART0_BASE,
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
						.gate_reg_base = SUNXI_CCU_BASE + UART_BGR_REG,
						.gate_reg_offset = SERIAL_DEFAULT_CLK_GATE_OFFSET(0),
						.rst_reg_base = SUNXI_CCU_BASE + UART_BGR_REG,
						.rst_reg_offset = SERIAL_DEFAULT_CLK_RST_OFFSET(0),
						.parent_clk = SERIAL_DEFAULT_PARENT_CLK,
				},
};

sunxi_serial_t uart_dbg = {
		.base = SUNXI_UART0_BASE,
		.id = 0,
		.baud_rate = UART_BAUDRATE_115200,
		.dlen = UART_DLEN_8,
		.stop = UART_STOP_BIT_0,
		.parity = UART_PARITY_NO,
		.gpio_pin =
				{
						.gpio_tx = {GPIO_PIN(GPIO_PORTH, 13), GPIO_PERIPH_MUX6},
						.gpio_rx = {GPIO_PIN(GPIO_PORTH, 14), GPIO_PERIPH_MUX6},
				},
		.uart_clk =
				{
						.gate_reg_base = SUNXI_CCU_BASE + UART_BGR_REG,
						.gate_reg_offset = SERIAL_DEFAULT_CLK_GATE_OFFSET(0),
						.rst_reg_base = SUNXI_CCU_BASE + UART_BGR_REG,
						.rst_reg_offset = SERIAL_DEFAULT_CLK_RST_OFFSET(0),
						.parent_clk = SERIAL_DEFAULT_PARENT_CLK,
				},
};

sunxi_i2c_t i2c_pmu = {
		.base = SUNXI_TWI0_BASE,
		.id = SUNXI_I2C0,
		.speed = SUNXI_I2C_SPEED_400K,
		.gpio =
				{
						.gpio_scl = {GPIO_PIN(GPIO_PORTL, 5), GPIO_PERIPH_MUX2},
						.gpio_sda = {GPIO_PIN(GPIO_PORTL, 6), GPIO_PERIPH_MUX2},
				},
		.i2c_clk =
				{
						.gate_reg_base = SUNXI_CCU_BASE + TWI_BGR_REG,
						.gate_reg_offset = TWI_DEFAULT_CLK_GATE_OFFSET(0),
						.rst_reg_base = SUNXI_CCU_BASE + TWI_BGR_REG,
						.rst_reg_offset = TWI_DEFAULT_CLK_RST_OFFSET(0),
						.parent_clk = 24000000,
				},
};

dram_para_t dram_para = {
		.dram_clk = 1056,
		.dram_type = 3,
		.dram_zq = 0x7b6bfb,
		.dram_odt_en = 0x1,
		.dram_para1 = 0x000010d2,
		.dram_para2 = 0x00000000,
		.dram_mr0 = 0x1c70,
		.dram_mr1 = 0x02,
		.dram_mr2 = 0x18,
		.dram_mr3 = 0x0,
		.dram_tpr0 = 0x004A2195,
		.dram_tpr1 = 0x02423190,
		.dram_tpr2 = 0x0008B061,
		.dram_tpr3 = 0xB4787896,
		.dram_tpr4 = 0x0,
		.dram_tpr5 = 0x48484848,
		.dram_tpr6 = 0x48,
		.dram_tpr7 = 0x1621121e,
		.dram_tpr8 = 0x0,
		.dram_tpr9 = 0x0,
		.dram_tpr10 = 0x0,
		.dram_tpr11 = 0x00460000,
		.dram_tpr12 = 0x00000055,
		.dram_tpr13 = 0x34010100,
};

void show_chip() {
	uint32_t chip_sid[4];
	chip_sid[0] = read32(SUNXI_SID_SRAM_BASE + 0x0);
	chip_sid[1] = read32(SUNXI_SID_SRAM_BASE + 0x4);
	chip_sid[2] = read32(SUNXI_SID_SRAM_BASE + 0x8);
	chip_sid[3] = read32(SUNXI_SID_SRAM_BASE + 0xc);

	printk_info("Model: AvaotaSBC Avaota F2 board.\n");
	printk_info("Core: XuanTie E907 RISC-V Core.\n");
	printk_info("Chip SID = %08x%08x%08x%08x\n", chip_sid[0], chip_sid[1], chip_sid[2], chip_sid[3]);
	printk_info("dump brom log:\n");
	printk_info("==================================\n");
	printk_info("%s", (char *) 0x00120000);
	printk_info("==================================\n");
}
