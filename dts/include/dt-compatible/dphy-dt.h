/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __DT_COMPATIBLE_DPHY_DT_H__
#define __DT_COMPATIBLE_DPHY_DT_H__

#include <driver.h>
#include <drivers/display/display.h>
#include <dt-compatible/dt-common.h>

#define SUNXI_DPHY_COMPATIBLE "allwinner,sunxi-combo-dphy"

/* drivers/display/dphy/dphy-combo.c */
const struct sunxi_dphy_variant *sunxi_dphy_variant_lookup(const char *name);

static inline __attribute__((always_inline)) int sunxi_dphy_dt_read_config(sunxi_dphy_t *phy, int node)
{
	const dt2c_fdt32_t *reg;
	const dt2c_fdt32_t *gate;
	const dt2c_fdt32_t *reset;
	const dt2c_fdt32_t *module_clock;
	const dt2c_fdt32_t *cells;
	const char *variant;
	int length;

	if (phy == NULL || node < 0 || !syterkit_dt_node_available(node) ||
		dt2c_fdt_node_check_compatible(DT2C_FDT_COMPILED_TREE, node, SUNXI_DPHY_COMPATIBLE) != 0)
		return DRIVER_ERROR_INVALID;
	*phy = (sunxi_dphy_t){ 0 };

	reg = syterkit_dt_cells(node, "reg", 2);
	gate = syterkit_dt_cells(node, "allwinner,clock-gate", 2);
	reset = syterkit_dt_cells(node, "allwinner,reset", 2);
	module_clock = syterkit_dt_cells(node, "allwinner,module-clock", 2);
	variant = (const char *)dt2c_fdt_getprop(DT2C_FDT_COMPILED_TREE, node, "allwinner,variant", &length);
	if (reg == NULL || dt2c_fdt32_to_cpu(reg[0]) == 0U || gate == NULL || variant == NULL ||
		length <= 1 || variant[length - 1] != '\0')
		return DRIVER_ERROR_INVALID;
	if (dt2c_fdt32_to_cpu(gate[1]) >= 32U || (reset != NULL && dt2c_fdt32_to_cpu(reset[1]) >= 32U))
		return DRIVER_ERROR_INVALID;

	phy->var = sunxi_dphy_variant_lookup(variant);
	if (phy->var == NULL)
		return DRIVER_ERROR_INVALID;

	phy->res.base = (uintptr_t)dt2c_fdt32_to_cpu(reg[0]);
	phy->res.gate.reg = (uintptr_t)dt2c_fdt32_to_cpu(gate[0]);
	phy->res.gate.bit = (uint8_t)dt2c_fdt32_to_cpu(gate[1]);
	if (reset != NULL) {
		phy->res.reset.reg = (uintptr_t)dt2c_fdt32_to_cpu(reset[0]);
		phy->res.reset.bit = (uint8_t)dt2c_fdt32_to_cpu(reset[1]);
	}
	if (module_clock != NULL) {
		if (dt2c_fdt32_to_cpu(module_clock[1]) > 7U)
			return DRIVER_ERROR_INVALID;
		phy->res.mod_reg = (uintptr_t)dt2c_fdt32_to_cpu(module_clock[0]);
		phy->res.mod_mux = (uint8_t)dt2c_fdt32_to_cpu(module_clock[1]);
		phy->res.mod_set = 1U;
	}
	cells = syterkit_dt_cells(node, "clock-frequency", 1);
	phy->res.mod_rate = cells != NULL ? dt2c_fdt32_to_cpu(cells[0]) : 0U;
	cells = syterkit_dt_cells(node, "allwinner,module-div", 1);
	phy->res.mod_div = cells != NULL ? (uint8_t)dt2c_fdt32_to_cpu(cells[0]) : 0U;
	cells = syterkit_dt_cells(node, "allwinner,phy-id", 1);
	phy->id = cells != NULL ? (uint8_t)dt2c_fdt32_to_cpu(cells[0]) : 0U;
	SYTERKIT_DT_TRACE_NODE("dphy", node);
	return DRIVER_OK;
}

#endif /* __DT_COMPATIBLE_DPHY_DT_H__ */
