/* SPDX-License-Identifier: GPL-2.0+ */

#include <dt-bindings/soc/sun8iw21.h>
#include <uart.h>

#include <drivers/serial/serial.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>
#include <drivers/clk/clk.h>

#include <common.h>
#include <drivers/dram/dram.h>

#include <config.h>
#include <efex.h>

static const sunxi_serial_t console = {
	.base = SUNXI_UART0_BASE,
	.id = 0,
	.uart_clk = {
		.gate_reg_base = (SUNXI_CCM_BASE + 0x90c),
		.gate_reg_offset = 0,
		.rst_reg_base = (SUNXI_CCM_BASE + 0x90c),
		.rst_reg_offset = 16,
		.parent_clk = 24000000,
	},
	.gpio_pin = {
		.gpio_tx = { .base = SUNXI_PIO_BASE, .pin = GPIO_PIN(GPIO_PORTH, 9), .bank = GPIO_PORTH - GPIO_PORTA, .mux = 5 },
		.gpio_rx = { .base = SUNXI_PIO_BASE, .pin = GPIO_PIN(GPIO_PORTH, 10), .bank = GPIO_PORTH - GPIO_PORTA, .mux = 5 },
	},
	.baud_rate = UART_BAUDRATE_115200,
	.parity = UART_PARITY_NO,
	.stop = UART_STOP_BIT_0,
	.dlen = UART_DLEN_8,
};

static sunxi_dram_t dram = {
	.parameters = {
		0x00000210,
		2,
		0x007b7bf9,
		0x00000000,
		0x000000d2,
		0x00000000,
		0x00000e73,
		0x00000002,
		0x00000000,
		0x00000000,
		0x00471992,
		0x0131a10c,
		0x00057041,
		0xb4787896,
		0x00000000,
		0x48484848,
		0x00000048,
		0x1621121e,
		0x00000000,
		0x00000000,
		0x00000000,
		0x00000022,
		0x00000077,
		0x34000100,
		0x00000000,
		0x00000000,
		0x00000000,
		0x00000000,
		0x00000000,
		0x00000000,
		0x00000000,
		0x00000000,
	},
	.parameter_count = 32,
	.memory_base = 0x40000000,
	.memory_size = 0x80000000U,
	.registers = {
		.ccu = { .base = SUNXI_CCU_BASE, .size = 0x1000 },
		.mctl_com = { .base = SUNXI_MCTL_COM_BASE, .size = 0x1000 },
		.mctl_phy = { .base = SUNXI_MCTL_PHY_BASE, .size = 0x1000 },
		.sysctrl = { .base = SUNXI_SYSCRL_BASE, .size = 0x1000 },
		.sid = { .base = SUNXI_SID_BASE, .size = 0x400 },
		.r_cpucfg = { .base = SUNXI_CPUS_CFG_BASE, .size = 0x400 },
		.r_prcm = { .base = SUNXI_RPRCM_BASE, .size = 0x400 },
	},
	.rtc = { .data_base = SUNXI_RTC_DATA_BASE, .data_size = 0x100 },
};

int main(void)
{
	uart_dbg = console;
	sunxi_serial_init(&uart_dbg);
	uart_log_console_ready();

	show_banner();

	sunxi_clk_init();

	if (sunxi_dram_init(&dram) == 0U) {
		pr_err("DRAM: initialization failed\n");
		return -1;
	}
	syterkit_efex_set_dram_result(dram.parameters, dram.parameter_count);

	return 0;
}
