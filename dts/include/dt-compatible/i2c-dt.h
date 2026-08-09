/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __DT_COMPATIBLE_I2C_DT_H__
#define __DT_COMPATIBLE_I2C_DT_H__

#include <driver.h>
#include <drivers/i2c.h>
#include <dt-compatible/pinctrl-dt.h>

static inline __attribute__((always_inline)) int
sunxi_i2c_dt_read_config(sunxi_i2c_t *i2c, int node) {
	const dt2c_fdt32_t *clock_gate;
	const dt2c_fdt32_t *pins;
	const dt2c_fdt32_t *reg;
	const dt2c_fdt32_t *reset;
	const dt2c_fdt32_t *value;
	sunxi_i2c_t config = {0};
	uint32_t id;

	if (i2c == NULL || node < 0 || !syterkit_dt_node_available(node) ||
	    dt2c_fdt_node_check_compatible(DT2C_FDT_COMPILED_TREE, node,
					   SUNXI_I2C_COMPATIBLE) != 0)
		return DRIVER_ERROR_INVALID;

	reg = syterkit_dt_cells(node, "reg", 2);
	value = syterkit_dt_cells(node, "allwinner,i2c-id", 1);
	if (reg == NULL || value == NULL)
		return DRIVER_ERROR_INVALID;
	config.base = (uintptr_t) dt2c_fdt32_to_cpu(reg[0]);
	config.dt_node = node;
	id = dt2c_fdt32_to_cpu(value[0]);
	if (config.base == 0U || id >= SUNXI_I2C_BUS_MAX)
		return DRIVER_ERROR_INVALID;
	config.id = (uint8_t) id;

	value = syterkit_dt_cells(node, "clock-frequency", 1);
	if (value == NULL || dt2c_fdt32_to_cpu(value[0]) == 0U)
		return DRIVER_ERROR_INVALID;
	config.speed = dt2c_fdt32_to_cpu(value[0]);

	value = syterkit_dt_cells(node, "allwinner,parent-clock-frequency", 1);
	clock_gate = syterkit_dt_cells(node, "allwinner,clock-gate", 2);
	reset = syterkit_dt_cells(node, "allwinner,reset", 2);
	if (value == NULL || clock_gate == NULL || reset == NULL)
		return DRIVER_ERROR_INVALID;
	config.i2c_clk.parent_clk = dt2c_fdt32_to_cpu(value[0]);
	config.i2c_clk.gate_reg_base =
			(uintptr_t) dt2c_fdt32_to_cpu(clock_gate[0]);
	config.i2c_clk.gate_reg_offset = dt2c_fdt32_to_cpu(clock_gate[1]);
	config.i2c_clk.rst_reg_base =
			(uintptr_t) dt2c_fdt32_to_cpu(reset[0]);
	config.i2c_clk.rst_reg_offset = dt2c_fdt32_to_cpu(reset[1]);
	if (config.i2c_clk.parent_clk == 0U ||
	    config.i2c_clk.gate_reg_base == 0U ||
	    config.i2c_clk.rst_reg_base == 0U ||
	    config.i2c_clk.gate_reg_offset >= 32U ||
	    config.i2c_clk.rst_reg_offset >= 32U)
		return DRIVER_ERROR_INVALID;

	pins = syterkit_dt_pinctrl_cells(node, 6);
	if (!syterkit_dt_gpio(pins, 0, &config.gpio.gpio_scl) ||
	    !syterkit_dt_gpio(pins, 3, &config.gpio.gpio_sda))
		return DRIVER_ERROR_INVALID;
	config.status = false;

	*i2c = config;
	return DRIVER_OK;
}

static inline __attribute__((always_inline)) int
sunxi_i2c_dt_read_alias(sunxi_i2c_t *i2c, const char *alias) {
	return sunxi_i2c_dt_read_config(
			i2c, syterkit_dt_alias_node(alias, SUNXI_I2C_COMPATIBLE));
}

#endif /* __DT_COMPATIBLE_I2C_DT_H__ */
