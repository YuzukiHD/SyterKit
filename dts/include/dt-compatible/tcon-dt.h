/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __DT_COMPATIBLE_TCON_DT_H__
#define __DT_COMPATIBLE_TCON_DT_H__

#include <driver.h>
#include <drivers/display/display.h>
#include <dt-compatible/display-res-dt.h>

#define SUNXI_TCON_COMPATIBLE	  "allwinner,sunxi-tcon-lcd"
#define SUNXI_TCON_TOP_COMPATIBLE "allwinner,sunxi-tcon-top"

/* Defined by the drivers: pick the register description by the allwinner,variant name. */
const struct sunxi_tcon_variant *sunxi_tcon_find_variant(const char *name);
const struct sunxi_tcon_top_variant *sunxi_tcon_top_find_variant(const char *name);

static inline __attribute__((always_inline)) int sunxi_tcon_dt_read_config(sunxi_tcon_t *tcon, int node)
{
	const dt2c_fdt32_t *id;
	const char *variant;

	if (tcon == NULL || node < 0 || !syterkit_dt_node_available(node) ||
		dt2c_fdt_node_check_compatible(DT2C_FDT_COMPILED_TREE, node, SUNXI_TCON_COMPATIBLE) != 0)
		return DRIVER_ERROR_INVALID;
	*tcon = (sunxi_tcon_t){ 0 };
	if (sunxi_disp_dt_read_res(&tcon->res, node, &variant) != DRIVER_OK || !tcon->res.mod_set)
		return DRIVER_ERROR_INVALID;
	tcon->var = sunxi_tcon_find_variant(variant);
	if (tcon->var == NULL)
		return DRIVER_ERROR_INVALID;
	id = syterkit_dt_cells(node, "allwinner,tcon-id", 1);
	tcon->id = id != NULL ? (uint8_t)dt2c_fdt32_to_cpu(id[0]) : 0U;
	SYTERKIT_DT_TRACE_NODE("tcon", node);
	return DRIVER_OK;
}

static inline __attribute__((always_inline)) int sunxi_tcon_top_dt_read_config(sunxi_tcon_top_t *top, int node)
{
	const char *variant;

	if (top == NULL || node < 0 || !syterkit_dt_node_available(node) ||
		dt2c_fdt_node_check_compatible(DT2C_FDT_COMPILED_TREE, node, SUNXI_TCON_TOP_COMPATIBLE) != 0)
		return DRIVER_ERROR_INVALID;
	*top = (sunxi_tcon_top_t){ 0 };
	if (sunxi_disp_dt_read_res(&top->res, node, &variant) != DRIVER_OK)
		return DRIVER_ERROR_INVALID;
	top->var = sunxi_tcon_top_find_variant(variant);
	if (top->var == NULL)
		return DRIVER_ERROR_INVALID;
	SYTERKIT_DT_TRACE_NODE("tcon-top", node);
	return DRIVER_OK;
}

#endif
