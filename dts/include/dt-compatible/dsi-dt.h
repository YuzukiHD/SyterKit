/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __DT_COMPATIBLE_DSI_DT_H__
#define __DT_COMPATIBLE_DSI_DT_H__

#include <driver.h>
#include <drivers/display/display.h>
#include <dt-compatible/dt-common.h>

#define SUNXI_DSI_COMPATIBLE "allwinner,sunxi-mipi-dsi"

/* drivers/display/dsi/dsi-host.c */
const struct sunxi_dsi_variant *sunxi_dsi_variant_lookup(const char *name);

static inline __attribute__((always_inline)) int sunxi_dsi_dt_read_config(sunxi_dsi_t *dsi, int node)
{
	const dt2c_fdt32_t *reg;
	const dt2c_fdt32_t *gate;
	const dt2c_fdt32_t *reset;
	const dt2c_fdt32_t *module_clock;
	const dt2c_fdt32_t *cells;
	const char *variant;
	int length;

	if (dsi == NULL || node < 0 || !syterkit_dt_node_available(node) ||
		dt2c_fdt_node_check_compatible(DT2C_FDT_COMPILED_TREE, node, SUNXI_DSI_COMPATIBLE) != 0)
		return DRIVER_ERROR_INVALID;
	*dsi = (sunxi_dsi_t){ 0 };

	reg = syterkit_dt_cells(node, "reg", 2);
	gate = syterkit_dt_cells(node, "allwinner,clock-gate", 2);
	reset = syterkit_dt_cells(node, "allwinner,reset", 2);
	module_clock = syterkit_dt_cells(node, "allwinner,module-clock", 2);
	variant = (const char *)dt2c_fdt_getprop(DT2C_FDT_COMPILED_TREE, node, "allwinner,variant", &length);
	if (reg == NULL || dt2c_fdt32_to_cpu(reg[0]) == 0U || gate == NULL || reset == NULL || variant == NULL ||
		length <= 1 || variant[length - 1] != '\0')
		return DRIVER_ERROR_INVALID;
	if (dt2c_fdt32_to_cpu(gate[1]) >= 32U || dt2c_fdt32_to_cpu(reset[1]) >= 32U)
		return DRIVER_ERROR_INVALID;

	dsi->var = sunxi_dsi_variant_lookup(variant);
	if (dsi->var == NULL)
		return DRIVER_ERROR_INVALID;

	dsi->res.base = (uintptr_t)dt2c_fdt32_to_cpu(reg[0]);
	dsi->res.gate.reg = (uintptr_t)dt2c_fdt32_to_cpu(gate[0]);
	dsi->res.gate.bit = (uint8_t)dt2c_fdt32_to_cpu(gate[1]);
	dsi->res.reset.reg = (uintptr_t)dt2c_fdt32_to_cpu(reset[0]);
	dsi->res.reset.bit = (uint8_t)dt2c_fdt32_to_cpu(reset[1]);
	if (module_clock != NULL) {
		if (dt2c_fdt32_to_cpu(module_clock[1]) > 7U)
			return DRIVER_ERROR_INVALID;
		dsi->res.mod_reg = (uintptr_t)dt2c_fdt32_to_cpu(module_clock[0]);
		dsi->res.mod_mux = (uint8_t)dt2c_fdt32_to_cpu(module_clock[1]);
		dsi->res.mod_set = 1U;
	}
	cells = syterkit_dt_cells(node, "clock-frequency", 1);
	dsi->res.mod_rate = cells != NULL ? dt2c_fdt32_to_cpu(cells[0]) : 0U;
	cells = syterkit_dt_cells(node, "allwinner,dsi-id", 1);
	dsi->id = cells != NULL ? (uint8_t)dt2c_fdt32_to_cpu(cells[0]) : 0U;
	SYTERKIT_DT_TRACE_NODE("dsi", node);
	return DRIVER_OK;
}

#endif /* __DT_COMPATIBLE_DSI_DT_H__ */
