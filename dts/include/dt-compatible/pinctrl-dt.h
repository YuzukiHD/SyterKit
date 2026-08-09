/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __DT_COMPATIBLE_PINCTRL_DT_H__
#define __DT_COMPATIBLE_PINCTRL_DT_H__

#include <dt-compatible/gpio-dt.h>

static inline __attribute__((always_inline)) const dt2c_fdt32_t *
syterkit_dt_pinctrl(int node, size_t *count, sunxi_gpio_t *controller) {
	const dt2c_fdt32_t *pinctrl_cells;
	const char *pinctrl_name;
	int controller_node;
	int length;
	int pinctrl;

	if (controller == NULL)
		return NULL;
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
	controller_node = dt2c_fdt_parent_offset(DT2C_FDT_COMPILED_TREE,
						  pinctrl);
	if (sunxi_gpio_dt_read_config(controller, controller_node) != DRIVER_OK)
		return NULL;
	pinctrl_cells = (const dt2c_fdt32_t *) dt2c_fdt_getprop(
			DT2C_FDT_COMPILED_TREE, pinctrl, "allwinner,pins",
			&length);
	if (pinctrl_cells == NULL || length <= 0 ||
	    length % (int) (SUNXI_GPIO_DT_CELLS * sizeof(*pinctrl_cells)) != 0)
		return NULL;
	if (count != NULL)
		*count = (size_t) length / sizeof(*pinctrl_cells);
	return pinctrl_cells;
}

static inline __attribute__((always_inline)) const dt2c_fdt32_t *
syterkit_dt_pinctrl_cells(int node, size_t count,
			  sunxi_gpio_t *controller) {
	const dt2c_fdt32_t *cells;
	size_t actual_count;

	cells = syterkit_dt_pinctrl(node, &actual_count, controller);
	return cells != NULL && actual_count == count ? cells : NULL;
}

static inline __attribute__((always_inline)) bool
syterkit_dt_pinctrl_gpio(const dt2c_fdt32_t *pins, size_t index,
			 const sunxi_gpio_t *controller, gpio_mux_t *gpio) {
	return sunxi_gpio_dt_read_pin(gpio, controller, pins + index);
}

#endif /* __DT_COMPATIBLE_PINCTRL_DT_H__ */
