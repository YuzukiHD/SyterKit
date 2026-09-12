/* SPDX-License-Identifier: GPL-2.0+ */

#include <dt-bindings/soc/sun300iw1.h>
#include <uart.h>

#include <common.h>
#include <log.h>
#include <drivers/clk/clk.h>
#include <drivers/dram/dram.h>
#include <drivers/serial/serial.h>
#include <efex.h>

static const sunxi_serial_t console = {
	.base = SUNXI_UART0_BASE,
	.id = 0,
	.uart_clk = {
		.gate_reg_base = (SUNXI_CCU_APP_BASE + 0x80),
		.gate_reg_offset = 15,
		.rst_reg_base = (SUNXI_CCU_APP_BASE + 0x90),
		.rst_reg_offset = 15,
		.parent_clk = 192000000,
	},
	.gpio_pin = {
		.gpio_tx = { .base = SUNXI_RPIO_BASE, .pin = GPIO_PIN(GPIO_PORTL, 4), .bank = GPIO_PORTL - GPIO_PORTL, .mux = 3 },
		.gpio_rx = { .base = SUNXI_RPIO_BASE, .pin = GPIO_PIN(GPIO_PORTL, 5), .bank = GPIO_PORTL - GPIO_PORTL, .mux = 3 },
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
		0x00400000,
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
		0x00000000,
		0x00000000,
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
	.memory_base = 0x80000000,
	.memory_size = 0x80000000U,
};

int main(void)
{
	sunxi_clk_preinit();
	uart_dbg = console;
	sunxi_serial_init(&uart_dbg);
	uart_log_console_ready();
	sunxi_clk_init();
	if (sunxi_dram_init(&dram) == 0U) {
		pr_err("DRAM: initialization failed\n");
		return -1;
	}
	syterkit_efex_set_dram_result(dram.parameters, dram.parameter_count);
	return 0;
}
