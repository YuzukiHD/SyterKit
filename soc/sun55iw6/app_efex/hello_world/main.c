/* SPDX-License-Identifier: GPL-2.0+ */

#include <dt-bindings/soc/sun55iw6.h>
#include <uart.h>

#include <log.h>
#include <drivers/serial/serial.h>

static const sunxi_serial_t console = {
	.base = SUNXI_UART0_BASE,
	.id = 0,
	.uart_clk = {
		.gate_reg_base = (SUNXI_CCU_BASE + 0xe00),
		.gate_reg_offset = 0,
		.rst_reg_base = (SUNXI_CCU_BASE + 0xe00),
		.rst_reg_offset = 16,
		.parent_clk = 24000000,
	},
	.gpio_pin = {
		.gpio_tx = { .base = SUNXI_GPIO_BASE, .pin = GPIO_PIN(GPIO_PORTB, 9), .bank = GPIO_PORTB - GPIO_PORTA, .mux = 2 },
		.gpio_rx = { .base = SUNXI_GPIO_BASE, .pin = GPIO_PIN(GPIO_PORTB, 10), .bank = GPIO_PORTB - GPIO_PORTA, .mux = 2 },
	},
	.baud_rate = UART_BAUDRATE_115200,
	.parity = UART_PARITY_NO,
	.stop = UART_STOP_BIT_0,
	.dlen = UART_DLEN_8,
};

int main(void)
{
	uart_dbg = console;
	sunxi_serial_init(&uart_dbg);
	uart_log_console_ready();
	pr_info("SyterKit eFEX hello world\n");
	return 0;
}
