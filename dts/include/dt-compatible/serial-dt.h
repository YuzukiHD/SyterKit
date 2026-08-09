/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __DT_COMPATIBLE_SERIAL_DT_H__
#define __DT_COMPATIBLE_SERIAL_DT_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <driver.h>
#include <drivers/serial.h>
#include <dt2c/dt.h>

static inline __attribute__((always_inline)) bool
sunxi_serial_dt_string_equal(const char *value, int length,
			     const char *expected, size_t expected_length) {
	return value != NULL && length == (int) expected_length + 1 &&
	       value[expected_length] == '\0' &&
	       __builtin_memcmp(value, expected, expected_length) == 0;
}

static inline __attribute__((always_inline)) const dt2c_fdt32_t *
sunxi_serial_dt_cells(int node, const char *name, size_t count) {
	const dt2c_fdt32_t *cells;
	int length;

	cells = (const dt2c_fdt32_t *) dt2c_fdt_getprop(
			DT2C_FDT_COMPILED_TREE, node, name, &length);
	if (cells == NULL || count > (size_t) 0x7fffffffU / sizeof(*cells) ||
	    length != (int) (count * sizeof(*cells)))
		return NULL;
	return cells;
}

static inline __attribute__((always_inline)) bool
sunxi_serial_dt_node_available(int node) {
	while (node >= 0) {
		const char *status;
		int length;

		status = (const char *) dt2c_fdt_getprop(
				DT2C_FDT_COMPILED_TREE, node, "status", &length);
		if (status != NULL &&
		    !sunxi_serial_dt_string_equal(status, length, "okay", 4) &&
		    !sunxi_serial_dt_string_equal(status, length, "ok", 2))
			return false;
		node = dt2c_fdt_parent_offset(DT2C_FDT_COMPILED_TREE, node);
	}

	return node == -DT2C_FDT_ERR_NOTFOUND;
}

static inline __attribute__((always_inline)) int
sunxi_serial_dt_stdout_node(void) {
	const char *stdout_path;
	int chosen;
	int length;
	int node;

	chosen = dt2c_fdt_path_offset(DT2C_FDT_COMPILED_TREE, "/chosen");
	if (chosen < 0)
		return chosen;
	stdout_path = (const char *) dt2c_fdt_getprop(
			DT2C_FDT_COMPILED_TREE, chosen, "stdout-path", &length);
	if (stdout_path == NULL || length <= 1 || stdout_path[length - 1] != '\0')
		return -DT2C_FDT_ERR_BADVALUE;

	node = dt2c_fdt_path_offset_namelen(DT2C_FDT_COMPILED_TREE,
					    stdout_path, length - 1);
	if (node < 0)
		return node;
	if (!sunxi_serial_dt_node_available(node) ||
	    dt2c_fdt_node_check_compatible(DT2C_FDT_COMPILED_TREE, node,
					   SUNXI_SERIAL_COMPATIBLE) != 0)
		return -DT2C_FDT_ERR_BADVALUE;
	return node;
}

static inline __attribute__((always_inline)) int
sunxi_serial_dt_read_config(sunxi_serial_t *uart) {
	const dt2c_fdt32_t *clock_gate;
	const dt2c_fdt32_t *pinctrl_cells;
	const dt2c_fdt32_t *pins;
	const dt2c_fdt32_t *reg;
	const dt2c_fdt32_t *reset;
	const dt2c_fdt32_t *value_cell;
	const char *pinctrl_name;
	const char *parity;
	sunxi_serial_t config = {0};
	uint32_t value;
	int length;
	int node;
	int pinctrl;

	if (uart == NULL)
		return DRIVER_ERROR_INVALID;
	node = sunxi_serial_dt_stdout_node();
	if (node < 0)
		return DRIVER_ERROR_INVALID;

	reg = sunxi_serial_dt_cells(node, "reg", 2);
	value_cell = sunxi_serial_dt_cells(node, "current-speed", 1);
	if (reg == NULL || value_cell == NULL)
		return DRIVER_ERROR_INVALID;
	config.base = (uintptr_t) dt2c_fdt32_to_cpu(reg[0]);
	value = dt2c_fdt32_to_cpu(value_cell[0]);
	if (config.base == 0U || value == 0U)
		return DRIVER_ERROR_INVALID;
	config.baud_rate = (sunxi_serial_baudrate_t) value;

	value_cell = sunxi_serial_dt_cells(node, "allwinner,uart-id", 1);
	if (value_cell == NULL || dt2c_fdt32_to_cpu(value_cell[0]) > 0xffU)
		return DRIVER_ERROR_INVALID;
	config.id = (uint8_t) dt2c_fdt32_to_cpu(value_cell[0]);

	value_cell = sunxi_serial_dt_cells(node, "clock-frequency", 1);
	clock_gate = sunxi_serial_dt_cells(node, "allwinner,clock-gate", 2);
	reset = sunxi_serial_dt_cells(node, "allwinner,reset", 2);
	if (value_cell == NULL || clock_gate == NULL || reset == NULL)
		return DRIVER_ERROR_INVALID;
	config.uart_clk.parent_clk = dt2c_fdt32_to_cpu(value_cell[0]);
	config.uart_clk.gate_reg_base =
			(uintptr_t) dt2c_fdt32_to_cpu(clock_gate[0]);
	config.uart_clk.gate_reg_offset = dt2c_fdt32_to_cpu(clock_gate[1]);
	config.uart_clk.rst_reg_base =
			(uintptr_t) dt2c_fdt32_to_cpu(reset[0]);
	config.uart_clk.rst_reg_offset = dt2c_fdt32_to_cpu(reset[1]);
	if (config.uart_clk.parent_clk == 0U ||
	    config.uart_clk.gate_reg_base == 0U ||
	    config.uart_clk.rst_reg_base == 0U ||
	    config.uart_clk.gate_reg_offset >= 32U ||
	    config.uart_clk.rst_reg_offset >= 32U)
		return DRIVER_ERROR_INVALID;

	pinctrl_name = (const char *) dt2c_fdt_getprop(
			DT2C_FDT_COMPILED_TREE, node, "pinctrl-names", &length);
	if (!sunxi_serial_dt_string_equal(pinctrl_name, length, "default", 7))
		return DRIVER_ERROR_INVALID;
	pinctrl_cells = sunxi_serial_dt_cells(node, "pinctrl-0", 1);
	if (pinctrl_cells == NULL)
		return DRIVER_ERROR_INVALID;
	pinctrl = dt2c_fdt_node_offset_by_phandle(
			DT2C_FDT_COMPILED_TREE,
			dt2c_fdt32_to_cpu(pinctrl_cells[0]));
	if (pinctrl < 0)
		return DRIVER_ERROR_INVALID;
	pins = sunxi_serial_dt_cells(pinctrl, "allwinner,pins", 6);
	if (pins == NULL)
		return DRIVER_ERROR_INVALID;
	if (dt2c_fdt32_to_cpu(pins[0]) > GPIO_PORTN ||
	    dt2c_fdt32_to_cpu(pins[1]) >= 32U ||
	    dt2c_fdt32_to_cpu(pins[2]) > GPIO_DISABLED ||
	    dt2c_fdt32_to_cpu(pins[3]) > GPIO_PORTN ||
	    dt2c_fdt32_to_cpu(pins[4]) >= 32U ||
	    dt2c_fdt32_to_cpu(pins[5]) > GPIO_DISABLED)
		return DRIVER_ERROR_INVALID;
	config.gpio_pin.gpio_tx.pin = GPIO_PIN(dt2c_fdt32_to_cpu(pins[0]),
					       dt2c_fdt32_to_cpu(pins[1]));
	config.gpio_pin.gpio_tx.mux = (uint8_t) dt2c_fdt32_to_cpu(pins[2]);
	config.gpio_pin.gpio_rx.pin = GPIO_PIN(dt2c_fdt32_to_cpu(pins[3]),
					       dt2c_fdt32_to_cpu(pins[4]));
	config.gpio_pin.gpio_rx.mux = (uint8_t) dt2c_fdt32_to_cpu(pins[5]);

	value_cell = sunxi_serial_dt_cells(node, "data-bits", 1);
	if (value_cell == NULL)
		return DRIVER_ERROR_INVALID;
	value = dt2c_fdt32_to_cpu(value_cell[0]);
	if (value < 5U || value > 8U)
		return DRIVER_ERROR_INVALID;
	config.dlen = (sunxi_serial_dlen_t) (value - 5U);

	value_cell = sunxi_serial_dt_cells(node, "stop-bits", 1);
	if (value_cell == NULL)
		return DRIVER_ERROR_INVALID;
	value = dt2c_fdt32_to_cpu(value_cell[0]);
	if (value < 1U || value > 2U)
		return DRIVER_ERROR_INVALID;
	config.stop = (sunxi_serial_stop_bit_t) (value - 1U);

	parity = (const char *) dt2c_fdt_getprop(
			DT2C_FDT_COMPILED_TREE, node, "parity", &length);
	if (sunxi_serial_dt_string_equal(parity, length, "none", 4))
		config.parity = UART_PARITY_NO;
	else if (sunxi_serial_dt_string_equal(parity, length, "odd", 3))
		config.parity = UART_PARITY_ODD;
	else if (sunxi_serial_dt_string_equal(parity, length, "even", 4))
		config.parity = UART_PARITY_EVEN;
	else
		return DRIVER_ERROR_INVALID;

	*uart = config;
	return DRIVER_OK;
}

#endif /* __DT_COMPATIBLE_SERIAL_DT_H__ */
