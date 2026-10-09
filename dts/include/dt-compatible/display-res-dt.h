/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __DT_COMPATIBLE_DISPLAY_RES_DT_H__
#define __DT_COMPATIBLE_DISPLAY_RES_DT_H__

#include <driver.h>
#include <drivers/display/display.h>
#include <dt-compatible/dt-common.h>

/*
 * Common resources of a display block node: reg, allwinner,clock-gate <reg bit>,
 * allwinner,reset <reg bit>, optional allwinner,module-clock <reg mux> and
 * clock-frequency, and the allwinner,variant name (returned in @variant).
 */
static inline __attribute__((always_inline)) int sunxi_disp_dt_read_res(
	sunxi_disp_res_t *res, int node, const char **variant)
{
	const dt2c_fdt32_t *clock_gate;
	const dt2c_fdt32_t *module_clock;
	const dt2c_fdt32_t *rate;
	const dt2c_fdt32_t *reg;
	const dt2c_fdt32_t *reset;
	const char *name;
	int length;

	reg = syterkit_dt_cells(node, "reg", 2);
	clock_gate = syterkit_dt_cells(node, "allwinner,clock-gate", 2);
	reset = syterkit_dt_cells(node, "allwinner,reset", 2);
	module_clock = syterkit_dt_cells(node, "allwinner,module-clock", 2);
	name = (const char *)dt2c_fdt_getprop(DT2C_FDT_COMPILED_TREE, node, "allwinner,variant", &length);
	if (reg == NULL || clock_gate == NULL || reset == NULL || name == NULL || length <= 1 ||
		name[length - 1] != '\0' || dt2c_fdt32_to_cpu(reg[0]) == 0U ||
		dt2c_fdt32_to_cpu(clock_gate[0]) == 0U || dt2c_fdt32_to_cpu(reset[0]) == 0U ||
		dt2c_fdt32_to_cpu(clock_gate[1]) >= 32U || dt2c_fdt32_to_cpu(reset[1]) >= 32U)
		return DRIVER_ERROR_INVALID;

	*res = (sunxi_disp_res_t){ 0 };
	res->base = (uintptr_t)dt2c_fdt32_to_cpu(reg[0]);
	res->gate.reg = (uintptr_t)dt2c_fdt32_to_cpu(clock_gate[0]);
	res->gate.bit = (uint8_t)dt2c_fdt32_to_cpu(clock_gate[1]);
	res->reset.reg = (uintptr_t)dt2c_fdt32_to_cpu(reset[0]);
	res->reset.bit = (uint8_t)dt2c_fdt32_to_cpu(reset[1]);
	if (module_clock != NULL) {
		if (dt2c_fdt32_to_cpu(module_clock[0]) == 0U || dt2c_fdt32_to_cpu(module_clock[1]) > 7U)
			return DRIVER_ERROR_INVALID;
		res->mod_reg = (uintptr_t)dt2c_fdt32_to_cpu(module_clock[0]);
		res->mod_mux = (uint8_t)dt2c_fdt32_to_cpu(module_clock[1]);
		res->mod_set = 1U;
	}
	rate = syterkit_dt_cells(node, "clock-frequency", 1);
	res->mod_rate = rate != NULL ? dt2c_fdt32_to_cpu(rate[0]) : 0U;
	if (variant != NULL)
		*variant = name;
	return DRIVER_OK;
}

#endif
