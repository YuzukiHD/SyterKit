/* SPDX-License-Identifier: GPL-2.0+ */

#include <dt-bindings/soc/sun252iw2.h>
#include <uart.h>

#include <common.h>
#include <drivers/clk/clk.h>
#include <drivers/psram/psram.h>
#include <drivers/serial/serial.h>
#include <efex.h>
#include <log.h>

static const sunxi_serial_t console = {
	.base = SUNXI_UART1_BASE,
	.id = 1,
	.uart_clk = {
		.gate_reg_base = SUNXI_CCU_BASE + 0x90c,
		.gate_reg_offset = 1,
		.rst_reg_base = SUNXI_CCU_BASE + 0x90c,
		.rst_reg_offset = 17,
		.parent_clk = 24000000,
	},
	.gpio_pin = {
		.gpio_tx = { .base = SUNXI_GPIO_BASE, .pin = GPIO_PIN(GPIO_PORTB, 0), .bank = GPIO_PORTB - GPIO_PORTA, .mux = 4 },
		.gpio_rx = { .base = SUNXI_GPIO_BASE, .pin = GPIO_PIN(GPIO_PORTB, 1), .bank = GPIO_PORTB - GPIO_PORTA, .mux = 4 },
	},
	.baud_rate = UART_BAUDRATE_115200,
	.parity = UART_PARITY_NO,
	.stop = UART_STOP_BIT_0,
	.dlen = UART_DLEN_8,
};

/* Match the board's PSRAM profile: 252 MHz, with the remaining fields auto-filled. */
static sunxi_psram_t psram = {
	.parameters = { 252U },
	.parameter_count = 32,
	.memory_base = SUNXI_PSRAM_BASE,
	.memory_size = 0x01000000U,
};

int main(void)
{
	uart_dbg = console;
	efex_param_load(&(const struct efex_param_targets){
		.uart = &uart_dbg,
		.psram = &psram,
	});
	sunxi_serial_init(&uart_dbg);
	uart_log_console_ready();
	sunxi_clk_init();

	uint32_t psram_size = sunxi_psram_init(&psram);
	efex_param_report_psram(&psram, psram_size);
	if (psram_size == 0U) {
		pr_err("PSRAM: initialization failed\n");
		return -1;
	}

	return 0;
}
