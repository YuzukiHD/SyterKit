/* SPDX-License-Identifier: GPL-2.0+ */

#include <dt2c/dt.h>
#include <dt-compatible/serial-dt.h>

#include "syter_test.h"

void test_case_main(const char *case_dir) {
	sunxi_serial_t uart;
	int disabled_uart;
	int stdout_uart;

	(void) case_dir;
	stdout_uart = dt2c_fdt_path_offset(DT2C_FDT_COMPILED_TREE,
					   "/soc/serial@2000");
	disabled_uart = dt2c_fdt_path_offset(DT2C_FDT_COMPILED_TREE,
					     "/disabled-bus/serial@1000");
	TEST_ASSERT(stdout_uart >= 0);
	TEST_ASSERT(disabled_uart >= 0);
	TEST_EQ(stdout_uart, sunxi_serial_dt_stdout_node());
	TEST_ASSERT(sunxi_serial_dt_node_available(stdout_uart));
	TEST_ASSERT(!sunxi_serial_dt_node_available(disabled_uart));

	TEST_EQ(DRIVER_OK, sunxi_serial_dt_read_config(&uart));
	TEST_EQ(0x2000U, uart.base);
	TEST_EQ(1U, uart.id);
	TEST_EQ(1500000U, uart.baud_rate);
	TEST_EQ(24000000U, uart.uart_clk.parent_clk);
	TEST_EQ(0x3000U, uart.uart_clk.gate_reg_base);
	TEST_EQ(1U, uart.uart_clk.gate_reg_offset);
	TEST_EQ(0x3000U, uart.uart_clk.rst_reg_base);
	TEST_EQ(17U, uart.uart_clk.rst_reg_offset);
	TEST_EQ(GPIO_PIN(GPIO_PORTH, 13), uart.gpio_pin.gpio_tx.pin);
	TEST_EQ(GPIO_PERIPH_MUX5, uart.gpio_pin.gpio_tx.mux);
	TEST_EQ(GPIO_PIN(GPIO_PORTH, 14), uart.gpio_pin.gpio_rx.pin);
	TEST_EQ(GPIO_PERIPH_MUX5, uart.gpio_pin.gpio_rx.mux);
	TEST_EQ(UART_DLEN_7, uart.dlen);
	TEST_EQ(UART_STOP_BIT_1, uart.stop);
	TEST_EQ(UART_PARITY_EVEN, uart.parity);
}
