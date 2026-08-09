/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __DT_COMPATIBLE_SERIAL_DT_H__
#define __DT_COMPATIBLE_SERIAL_DT_H__

#include <driver.h>
#include <drivers/serial.h>
#include <dt-compatible/pinctrl-dt.h>

static inline __attribute__((always_inline)) bool
sunxi_serial_dt_string_equal(const char *value, int length,
			     const char *expected, size_t expected_length) {
	return syterkit_dt_string_equal(value, length, expected, expected_length);
}

static inline __attribute__((always_inline)) const dt2c_fdt32_t *
sunxi_serial_dt_cells(int node, const char *name, size_t count) {
	return syterkit_dt_cells(node, name, count);
}

static inline __attribute__((always_inline)) bool
sunxi_serial_dt_node_available(int node) {
	return syterkit_dt_node_available(node);
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
	const dt2c_fdt32_t *pins;
	const dt2c_fdt32_t *reg;
	const dt2c_fdt32_t *reset;
	const dt2c_fdt32_t *value_cell;
	const char *parity;
	sunxi_serial_t config;
	uint32_t value;
	int length;
	int node;

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

	pins = syterkit_dt_pinctrl_cells(node, 6);
	if (!syterkit_dt_gpio(pins, 0, &config.gpio_pin.gpio_tx) ||
	    !syterkit_dt_gpio(pins, 3, &config.gpio_pin.gpio_rx))
		return DRIVER_ERROR_INVALID;

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
