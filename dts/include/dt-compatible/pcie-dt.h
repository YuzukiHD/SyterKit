/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __DT_COMPATIBLE_PCIE_DT_H__
#define __DT_COMPATIBLE_PCIE_DT_H__

#include <stdint.h>

#include <driver.h>
#include <drivers/pcie/pcie.h>
#include <dt-compatible/dt-common.h>
#include <dt-compatible/gpio-dt.h>

#define SUNXI_PCIE_RC_COMPATIBLE        "allwinner,sun55iw6-pcie-rc"
#define SUNXI_PCIE_EP_COMPATIBLE        "allwinner,sun55iw6-pcie-ep"

static inline __attribute__((always_inline)) int sunxi_pcie_dt_mode(
	int node, enum pcie_mode *mode)
{
	if (mode == NULL || node < 0 || !syterkit_dt_node_available(node))
		return DRIVER_ERROR_INVALID;
	if (dt2c_fdt_node_check_compatible(DT2C_FDT_COMPILED_TREE, node,
		SUNXI_PCIE_RC_COMPATIBLE) == 0) {
		*mode = PCIE_MODE_RC;
		return DRIVER_OK;
	}
	if (dt2c_fdt_node_check_compatible(DT2C_FDT_COMPILED_TREE, node,
		SUNXI_PCIE_EP_COMPATIBLE) == 0) {
		*mode = PCIE_MODE_EP;
		return DRIVER_OK;
	}
	return DRIVER_ERROR_INVALID;
}

static inline __attribute__((always_inline)) int sunxi_pcie_dt_read_config(
	struct pcie_config *config, int node)
{
	const dt2c_fdt32_t *reg;
	const dt2c_fdt32_t *cells;
	const dt2c_fdt32_t *value;
	const dt2c_fdt32_t *reset_gpio;
	const void *reset_gpio_property;
	int length;
	struct pcie_config parsed;
	uint32_t dbi_base;
	uint32_t dbi_size;

	if (config == NULL || sunxi_pcie_dt_mode(node, &parsed.mode) != DRIVER_OK)
		return DRIVER_ERROR_INVALID;
	pcie_config_sun55iw6(&parsed, parsed.mode);

	reg = (const dt2c_fdt32_t *)dt2c_fdt_getprop(DT2C_FDT_COMPILED_TREE, node,
		"reg", &length);
	if (reg == NULL || (length != 8 && length != 16))
		return DRIVER_ERROR_INVALID;
	if (length == 8) {
		dbi_base = dt2c_fdt32_to_cpu(reg[0]);
		dbi_size = dt2c_fdt32_to_cpu(reg[1]);
	} else {
		if (dt2c_fdt32_to_cpu(reg[0]) != 0U || dt2c_fdt32_to_cpu(reg[2]) != 0U)
			return DRIVER_ERROR_INVALID;
		dbi_base = dt2c_fdt32_to_cpu(reg[1]);
		dbi_size = dt2c_fdt32_to_cpu(reg[3]);
	}
	if (dbi_base == 0U || dbi_size == 0U)
		return DRIVER_ERROR_INVALID;
	parsed.controller.dbi_base = (uintptr_t)dbi_base;
	parsed.controller.dbi_size = dbi_size;
	if ((uintptr_t)dbi_base > ~(uintptr_t)0 - 0x400000U)
		return DRIVER_ERROR_INVALID;
	parsed.controller.app_base = (uintptr_t)dbi_base + 0x400000U;

	value = syterkit_dt_cells(node, "allwinner,app-base", 1);
	if (value != NULL)
		parsed.controller.app_base = (uintptr_t)dt2c_fdt32_to_cpu(value[0]);
	value = syterkit_dt_cells(node, "allwinner,phy-subsys-base", 1);
	if (value != NULL)
		parsed.phy.subsys_base = (uintptr_t)dt2c_fdt32_to_cpu(value[0]);
	value = syterkit_dt_cells(node, "allwinner,phy-base", 1);
	if (value != NULL)
		parsed.phy.phy_base = (uintptr_t)dt2c_fdt32_to_cpu(value[0]);

	cells = syterkit_dt_cells(node, "allwinner,config-window", 3);
	if (cells != NULL) {
		parsed.controller.cfg_cpu_addr = (uintptr_t)dt2c_fdt32_to_cpu(cells[0]);
		parsed.controller.cfg_pci_addr = dt2c_fdt32_to_cpu(cells[1]);
		parsed.controller.cfg_size = dt2c_fdt32_to_cpu(cells[2]);
	}
	cells = syterkit_dt_cells(node, "allwinner,io-window", 3);
	if (cells != NULL) {
		parsed.controller.io_cpu_addr = (uintptr_t)dt2c_fdt32_to_cpu(cells[0]);
		parsed.controller.io_pci_addr = dt2c_fdt32_to_cpu(cells[1]);
		parsed.controller.io_size = dt2c_fdt32_to_cpu(cells[2]);
	}
	cells = syterkit_dt_cells(node, "allwinner,mem-window", 3);
	if (cells != NULL) {
		parsed.controller.mem_cpu_addr = (uintptr_t)dt2c_fdt32_to_cpu(cells[0]);
		parsed.controller.mem_pci_addr = dt2c_fdt32_to_cpu(cells[1]);
		parsed.controller.mem_size = dt2c_fdt32_to_cpu(cells[2]);
	}

	value = syterkit_dt_cells(node, "num-lanes", 1);
	if (value != NULL)
		parsed.controller.lanes = (uint8_t)dt2c_fdt32_to_cpu(value[0]);
	value = syterkit_dt_cells(node, "max-link-speed", 1);
	if (value != NULL)
		parsed.controller.link_gen = (uint8_t)dt2c_fdt32_to_cpu(value[0]);
	value = syterkit_dt_cells(node, "allwinner,timeout-us", 1);
	if (value != NULL) {
		parsed.controller.timeout_us = dt2c_fdt32_to_cpu(value[0]);
		parsed.phy.timeout_us = parsed.controller.timeout_us;
	}
	reset_gpio_property = dt2c_fdt_getprop(DT2C_FDT_COMPILED_TREE, node,
		"reset-gpios", &length);
	if (reset_gpio_property != NULL) {
		if (length != (int)(SUNXI_GPIO_DT_SPECIFIER_CELLS *
			 sizeof(*reset_gpio)))
			return DRIVER_ERROR_INVALID;
		reset_gpio = (const dt2c_fdt32_t *)reset_gpio_property;
		if (!sunxi_gpio_dt_read_specifier(&parsed.reset_gpio, reset_gpio))
			return DRIVER_ERROR_INVALID;
		parsed.has_reset_gpio = true;
	}
	if (parsed.controller.app_base == 0U || parsed.phy.subsys_base == 0U ||
	    parsed.phy.phy_base == 0U || parsed.controller.lanes == 0U ||
	    parsed.controller.link_gen == 0U || parsed.controller.timeout_us == 0U)
		return DRIVER_ERROR_INVALID;

	*config = parsed;
	SYTERKIT_DT_TRACE_NODE("pcie", node);
	SYTERKIT_DT_TRACE("pcie config mode=%u dbi=%p app=%p lanes=%u gen=%u\n",
		config->mode, (void *)config->controller.dbi_base,
		(void *)config->controller.app_base, config->controller.lanes,
		config->controller.link_gen);
	return DRIVER_OK;
}

static inline __attribute__((always_inline)) int sunxi_pcie_dt_read_alias(
	struct pcie_config *config, const char *alias)
{
	int node;

	if (config == NULL || alias == NULL)
		return DRIVER_ERROR_INVALID;
	node = dt2c_fdt_alias_node_offset(DT2C_FDT_COMPILED_TREE, alias);
	return sunxi_pcie_dt_read_config(config, node);
}

#endif /* __DT_COMPATIBLE_PCIE_DT_H__ */
