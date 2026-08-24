/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __DT_COMPATIBLE_GPIO_DT_H__
#define __DT_COMPATIBLE_GPIO_DT_H__

#include <driver.h>
#include <drivers/gpio/gpio.h>
#include <dt-compatible/dt-common.h>

#define SUNXI_GPIO_DT_CELLS 3U
#define SUNXI_GPIO_DT_SPECIFIER_CELLS (SUNXI_GPIO_DT_CELLS + 1U)

static inline __attribute__((always_inline)) int sunxi_gpio_dt_read_config(sunxi_gpio_t *gpio, int node)
{
	const dt2c_fdt32_t *gpio_cells;
	const dt2c_fdt32_t *bank_base;
	const dt2c_fdt32_t *bank_count;
	const dt2c_fdt32_t *reg;
	const void *gpio_controller;
	sunxi_gpio_t config = { 0 };
	uint32_t first_bank;
	uint32_t number_of_banks;
	int length;

	if (gpio == NULL || node < 0 || !syterkit_dt_node_available(node) || dt2c_fdt_node_check_compatible(DT2C_FDT_COMPILED_TREE, node, SUNXI_PINCTRL_COMPATIBLE) != 0)
		return DRIVER_ERROR_INVALID;

	reg = syterkit_dt_cells(node, "reg", 2);
	gpio_cells = syterkit_dt_cells(node, "#gpio-cells", 1);
	bank_base = syterkit_dt_cells(node, "allwinner,bank-base", 1);
	bank_count = syterkit_dt_cells(node, "allwinner,bank-count", 1);
	gpio_controller = dt2c_fdt_getprop(DT2C_FDT_COMPILED_TREE, node, "gpio-controller", &length);
	if (reg == NULL || gpio_cells == NULL || bank_base == NULL || bank_count == NULL || gpio_controller == NULL || length != 0 ||
	    dt2c_fdt32_to_cpu(gpio_cells[0]) != SUNXI_GPIO_DT_CELLS || dt2c_fdt32_to_cpu(reg[0]) == 0U || dt2c_fdt32_to_cpu(reg[1]) == 0U)
		return DRIVER_ERROR_INVALID;
	first_bank = dt2c_fdt32_to_cpu(bank_base[0]);
	number_of_banks = dt2c_fdt32_to_cpu(bank_count[0]);
	if (first_bank > GPIO_PORTN || number_of_banks == 0U || number_of_banks > (uint32_t)GPIO_PORTN + 1U - first_bank)
		return DRIVER_ERROR_INVALID;

	config.dt_node = node;
	config.base = (uintptr_t)dt2c_fdt32_to_cpu(reg[0]);
	config.bank_base = (uint8_t)first_bank;
	config.bank_count = (uint8_t)number_of_banks;
	*gpio = config;
	SYTERKIT_DT_TRACE_NODE("gpio", node);
	SYTERKIT_DT_TRACE("gpio config base=%p first_bank=%u bank_count=%u\n", (void *)gpio->base, gpio->bank_base, gpio->bank_count);
	return DRIVER_OK;
}

static inline __attribute__((always_inline)) int sunxi_gpio_dt_read_alias(sunxi_gpio_t *gpio, const char *alias)
{
	if (alias == NULL)
		return DRIVER_ERROR_INVALID;
	return sunxi_gpio_dt_read_config(gpio, syterkit_dt_alias_node(alias, SUNXI_PINCTRL_COMPATIBLE));
}

static inline __attribute__((always_inline)) bool sunxi_gpio_dt_read_pin(gpio_mux_t *gpio, const sunxi_gpio_t *controller, const dt2c_fdt32_t *cells)
{
	uint32_t mux;
	uint32_t pin;
	uint32_t port;

	if (gpio == NULL || controller == NULL || cells == NULL)
		return false;
	port = dt2c_fdt32_to_cpu(cells[0]);
	pin = dt2c_fdt32_to_cpu(cells[1]);
	mux = dt2c_fdt32_to_cpu(cells[2]);
	if (port < controller->bank_base || port >= (uint32_t)controller->bank_base + controller->bank_count || pin >= 32U || mux > GPIO_DISABLED)
		return false;

	gpio->base = controller->base;
	gpio->pin = GPIO_PIN(port, pin);
	gpio->bank = (uint8_t)(port - controller->bank_base);
	gpio->mux = (uint8_t)mux;
	return true;
}

static inline __attribute__((always_inline)) bool sunxi_gpio_dt_read_specifier(gpio_mux_t *gpio, const dt2c_fdt32_t *cells)
{
	sunxi_gpio_t controller;
	int controller_node;

	if (gpio == NULL || cells == NULL)
		return false;
	controller_node = dt2c_fdt_node_offset_by_phandle(DT2C_FDT_COMPILED_TREE, dt2c_fdt32_to_cpu(cells[0]));
	if (sunxi_gpio_dt_read_config(&controller, controller_node) != DRIVER_OK)
		return false;
	return sunxi_gpio_dt_read_pin(gpio, &controller, cells + 1);
}

static inline __attribute__((always_inline)) bool sunxi_gpio_dt_read_property(gpio_mux_t *gpio, int node, const char *name)
{
	if (gpio == NULL || node < 0 || name == NULL)
		return false;
	return sunxi_gpio_dt_read_specifier(gpio, syterkit_dt_cells(node, name, SUNXI_GPIO_DT_SPECIFIER_CELLS));
}

#endif /* __DT_COMPATIBLE_GPIO_DT_H__ */
