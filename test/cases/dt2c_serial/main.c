/* SPDX-License-Identifier: GPL-2.0+ */

#include <dt2c/dt.h>
#include <dt-compatible/serial-dt.h>

#include "syter_test.h"

void test_case_main(const char *case_dir)
{
	sunxi_serial_t primary;
	sunxi_serial_t secondary;
	sunxi_serial_t stdout_uart_config;
	int disabled_uart;
	int stdout_uart;

	(void)case_dir;
	stdout_uart = dt2c_fdt_path_offset(DT2C_FDT_COMPILED_TREE, "/soc/serial@2000");
	disabled_uart = dt2c_fdt_path_offset(DT2C_FDT_COMPILED_TREE, "/disabled-bus/serial@1000");
	TEST_ASSERT(stdout_uart >= 0);
	TEST_ASSERT(disabled_uart >= 0);
	TEST_EQ(stdout_uart, sunxi_serial_dt_stdout_node());
	TEST_ASSERT(sunxi_serial_dt_node_available(stdout_uart));
	TEST_ASSERT(!sunxi_serial_dt_node_available(disabled_uart));
	TEST_EQ(DRIVER_ERROR_INVALID, sunxi_serial_dt_read_config(&primary, disabled_uart));
	TEST_EQ(DRIVER_ERROR_INVALID, sunxi_serial_dt_read_alias(&primary, "uart-disabled"));
	TEST_EQ(DRIVER_ERROR_INVALID, sunxi_serial_dt_read_alias(&primary, "missing"));
	TEST_EQ(DRIVER_ERROR_INVALID, sunxi_serial_dt_read_alias(&primary, NULL));

	TEST_EQ(DRIVER_OK, sunxi_serial_dt_read_config(&primary, stdout_uart));
	TEST_EQ(DRIVER_OK, sunxi_serial_dt_read_stdout(&stdout_uart_config));
	TEST_EQ(DRIVER_OK, sunxi_serial_dt_read_alias(&secondary, "uart-secondary"));
	TEST_EQ(primary.base, stdout_uart_config.base);
	TEST_EQ(primary.id, stdout_uart_config.id);
	TEST_EQ(0x2000U, primary.base);
	TEST_EQ(1U, primary.id);
	TEST_EQ(1500000U, primary.baud_rate);
	TEST_EQ(24000000U, primary.uart_clk.parent_clk);
	TEST_EQ(0x3000U, primary.uart_clk.gate_reg_base);
	TEST_EQ(1U, primary.uart_clk.gate_reg_offset);
	TEST_EQ(0x3000U, primary.uart_clk.rst_reg_base);
	TEST_EQ(17U, primary.uart_clk.rst_reg_offset);
	TEST_EQ(GPIO_PIN(GPIO_PORTH, 13), primary.gpio_pin.gpio_tx.pin);
	TEST_EQ(GPIO_PERIPH_MUX5, primary.gpio_pin.gpio_tx.mux);
	TEST_EQ(0x02000000U, primary.gpio_pin.gpio_tx.base);
	TEST_EQ(7, primary.gpio_pin.gpio_tx.bank);
	TEST_EQ(GPIO_PIN(GPIO_PORTH, 14), primary.gpio_pin.gpio_rx.pin);
	TEST_EQ(GPIO_PERIPH_MUX5, primary.gpio_pin.gpio_rx.mux);
	TEST_EQ(UART_DLEN_7, primary.dlen);
	TEST_EQ(UART_STOP_BIT_1, primary.stop);
	TEST_EQ(UART_PARITY_EVEN, primary.parity);

	TEST_EQ(0x3000U, secondary.base);
	TEST_EQ(2U, secondary.id);
	TEST_EQ(57600U, secondary.baud_rate);
	TEST_EQ(40000000U, secondary.uart_clk.parent_clk);
	TEST_EQ(GPIO_PIN(GPIO_PORTB, 8), secondary.gpio_pin.gpio_tx.pin);
	TEST_EQ(GPIO_PERIPH_MUX6, secondary.gpio_pin.gpio_tx.mux);
	TEST_EQ(GPIO_PIN(GPIO_PORTB, 9), secondary.gpio_pin.gpio_rx.pin);
	TEST_EQ(GPIO_PERIPH_MUX6, secondary.gpio_pin.gpio_rx.mux);
	TEST_EQ(UART_DLEN_8, secondary.dlen);
	TEST_EQ(UART_STOP_BIT_0, secondary.stop);
	TEST_EQ(UART_PARITY_ODD, secondary.parity);
}
