/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __DT_COMPATIBLE_PINCTRL_DT_H__
#define __DT_COMPATIBLE_PINCTRL_DT_H__

#include <drivers/gpio.h>
#include <dt-compatible/dt-common.h>

static inline __attribute__((always_inline)) const dt2c_fdt32_t *
syterkit_dt_pinctrl_cells(int node, size_t count) {
	const dt2c_fdt32_t *pinctrl_cells;
	const char *pinctrl_name;
	int length;
	int pinctrl;

	pinctrl_name = (const char *) dt2c_fdt_getprop(
			DT2C_FDT_COMPILED_TREE, node, "pinctrl-names", &length);
	if (!syterkit_dt_string_equal(pinctrl_name, length, "default", 7))
		return NULL;
	pinctrl_cells = syterkit_dt_cells(node, "pinctrl-0", 1);
	if (pinctrl_cells == NULL)
		return NULL;
	pinctrl = dt2c_fdt_node_offset_by_phandle(
			DT2C_FDT_COMPILED_TREE,
			dt2c_fdt32_to_cpu(pinctrl_cells[0]));
	if (pinctrl < 0 || !syterkit_dt_node_available(pinctrl))
		return NULL;
	return syterkit_dt_cells(pinctrl, "allwinner,pins", count);
}

static inline __attribute__((always_inline)) bool
syterkit_dt_gpio(const dt2c_fdt32_t *pins, size_t index, gpio_mux_t *gpio) {
	uint32_t mux;
	uint32_t pin;
	uint32_t port;

	if (pins == NULL || gpio == NULL)
		return false;
	port = dt2c_fdt32_to_cpu(pins[index]);
	pin = dt2c_fdt32_to_cpu(pins[index + 1]);
	mux = dt2c_fdt32_to_cpu(pins[index + 2]);
	if (port > GPIO_PORTN || pin >= 32U || mux > GPIO_DISABLED)
		return false;
	gpio->pin = GPIO_PIN(port, pin);
	gpio->mux = (uint8_t) mux;
	return true;
}

#endif /* __DT_COMPATIBLE_PINCTRL_DT_H__ */
