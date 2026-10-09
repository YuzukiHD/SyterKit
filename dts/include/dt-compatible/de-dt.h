/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __DT_COMPATIBLE_DE_DT_H__
#define __DT_COMPATIBLE_DE_DT_H__

#include <driver.h>
#include <drivers/display/display.h>
#include <dt-compatible/dt-common.h>

#define SUNXI_DE_COMPATIBLE "allwinner,sunxi-de"

/* Defined by the driver: picks the register description by the allwinner,variant name. */
const struct sunxi_de_variant *sunxi_de_variant_find(const char *name);

static inline __attribute__((always_inline)) int sunxi_de_dt_read_config(sunxi_de_t *de, int node)
{
	const dt2c_fdt32_t *clock_gate;
	const dt2c_fdt32_t *module_clock;
	const dt2c_fdt32_t *port;
	const dt2c_fdt32_t *rate;
	const dt2c_fdt32_t *reg;
	const dt2c_fdt32_t *reset;
	const char *variant;
	int length;

	if (de == NULL || node < 0 || !syterkit_dt_node_available(node) ||
		dt2c_fdt_node_check_compatible(DT2C_FDT_COMPILED_TREE, node, SUNXI_DE_COMPATIBLE) != 0)
		return DRIVER_ERROR_INVALID;

	reg = syterkit_dt_cells(node, "reg", 2);
	clock_gate = syterkit_dt_cells(node, "allwinner,clock-gate", 2);
	reset = syterkit_dt_cells(node, "allwinner,reset", 2);
	module_clock = syterkit_dt_cells(node, "allwinner,module-clock", 2);
	variant = (const char *)dt2c_fdt_getprop(DT2C_FDT_COMPILED_TREE, node, "allwinner,variant", &length);
	if (reg == NULL || clock_gate == NULL || reset == NULL || module_clock == NULL || variant == NULL ||
		length <= 1 || variant[length - 1] != '\0')
		return DRIVER_ERROR_INVALID;
	if (dt2c_fdt32_to_cpu(reg[0]) == 0U || dt2c_fdt32_to_cpu(clock_gate[0]) == 0U ||
		dt2c_fdt32_to_cpu(reset[0]) == 0U || dt2c_fdt32_to_cpu(module_clock[0]) == 0U ||
		dt2c_fdt32_to_cpu(clock_gate[1]) >= 32U || dt2c_fdt32_to_cpu(reset[1]) >= 32U ||
		dt2c_fdt32_to_cpu(module_clock[1]) > 7U)
		return DRIVER_ERROR_INVALID;

	*de = (sunxi_de_t){ 0 };
	de->var = sunxi_de_variant_find(variant);
	if (de->var == NULL)
		return DRIVER_ERROR_INVALID;

	de->res.base = (uintptr_t)dt2c_fdt32_to_cpu(reg[0]);
	de->res.gate.reg = (uintptr_t)dt2c_fdt32_to_cpu(clock_gate[0]);
	de->res.gate.bit = (uint8_t)dt2c_fdt32_to_cpu(clock_gate[1]);
	de->res.reset.reg = (uintptr_t)dt2c_fdt32_to_cpu(reset[0]);
	de->res.reset.bit = (uint8_t)dt2c_fdt32_to_cpu(reset[1]);
	de->res.mod_reg = (uintptr_t)dt2c_fdt32_to_cpu(module_clock[0]);
	de->res.mod_mux = (uint8_t)dt2c_fdt32_to_cpu(module_clock[1]);
	de->res.mod_set = 1U;
	rate = syterkit_dt_cells(node, "clock-frequency", 1);
	de->res.mod_rate = rate != NULL ? dt2c_fdt32_to_cpu(rate[0]) : 0U;
	port = syterkit_dt_cells(node, "allwinner,de-port", 1);
	if (port != NULL)
		de->port = (uint8_t)dt2c_fdt32_to_cpu(port[0]);
	SYTERKIT_DT_TRACE_NODE("de", node);
	return DRIVER_OK;
}

static inline __attribute__((always_inline)) int sunxi_de_dt_read_alias(sunxi_de_t *de, const char *alias)
{
	int node;

	if (alias == NULL)
		return DRIVER_ERROR_INVALID;
	node = syterkit_dt_alias_node(alias, SUNXI_DE_COMPATIBLE);
	if (node < 0)
		return DRIVER_ERROR_INVALID;
	return sunxi_de_dt_read_config(de, node);
}

#endif /* __DT_COMPATIBLE_DE_DT_H__ */
