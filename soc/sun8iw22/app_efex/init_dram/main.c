/* SPDX-License-Identifier: GPL-2.0+ */

#include <dt-bindings/soc/sun8iw22.h>
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
		.gate_reg_base = (SUNXI_CCM_BASE + 0xe00),
		.gate_reg_offset = 0,
		.rst_reg_base = (SUNXI_CCM_BASE + 0xe00),
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

static sunxi_dram_t dram = {
	.parameters = {
		0x000004b0,
		4,
		0x00000808,
		0x00000c0c,
		0x000c0c0c,
		0x00006060,
		0x000060fa,
		0x02000001,
		0x00000964,
		0x00000101,
		0x00000018,
		0x00000000,
		0x00000000,
		0x00000400,
		0x00000813,
		0x00000000,
		0x00000000,
		0x00000000,
		0x00000000,
		0x00000000,
		0x00000000,
		0x00000000,
		0x00000000,
		0x00000000,
		0x00000000,
		0x00000000,
		0x00004000,
		0x00002500,
		0x00005555,
		0x0000201f,
		0x00007071,
		0x010700f5,
	},
	.parameter_count = 32,
	.memory_base = 0x40000000,
	.memory_size = 0x80000000U,
};

int main(void)
{
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
