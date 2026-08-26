/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __DT_COMPATIBLE_SID_DT_H__
#define __DT_COMPATIBLE_SID_DT_H__

#include <driver.h>
#include <drivers/sid/sid.h>
#include <dt-compatible/dt-common.h>

static inline __attribute__((always_inline)) int sunxi_sid_dt_read_config(sunxi_sid_t *sid, int node)
{
	const dt2c_fdt32_t *hv_switch;
	const dt2c_fdt32_t *reg;
	const dt2c_fdt32_t *sram_base;
	sunxi_sid_t config = { 0 };
	int hv_switch_length;
	int sram_base_length;

	if (sid == NULL || node < 0 || !syterkit_dt_node_available(node) || dt2c_fdt_node_check_compatible(DT2C_FDT_COMPILED_TREE, node, SUNXI_SID_COMPATIBLE) != 0)
		return DRIVER_ERROR_INVALID;

	reg = syterkit_dt_cells(node, "reg", 2);
	if (reg == NULL)
		return DRIVER_ERROR_INVALID;

	config.dt_node = node;
	config.base = (uintptr_t)dt2c_fdt32_to_cpu(reg[0]);
	config.size = dt2c_fdt32_to_cpu(reg[1]);
	if (config.base == 0U || config.size < SUNXI_SID_SRAM_OFFSET + sizeof(uint32_t) ||
	    (config.base & (sizeof(uint32_t) - 1U)) != 0U ||
	    config.base + (uintptr_t)config.size < config.base)
		return DRIVER_ERROR_INVALID;

	sram_base = (const dt2c_fdt32_t *)dt2c_fdt_getprop(DT2C_FDT_COMPILED_TREE, node, "allwinner,sid-sram-base", &sram_base_length);
	if (sram_base == NULL) {
		if (sram_base_length != -DT2C_FDT_ERR_NOTFOUND)
			return DRIVER_ERROR_INVALID;
		config.sram_base = config.base + SUNXI_SID_SRAM_OFFSET;
		if (config.sram_base < config.base)
			return DRIVER_ERROR_INVALID;
	} else {
		if (sram_base_length != (int)sizeof(*sram_base))
			return DRIVER_ERROR_INVALID;
		config.sram_base = (uintptr_t)dt2c_fdt32_to_cpu(sram_base[0]);
		if (config.sram_base == 0U || (config.sram_base & (sizeof(uint32_t) - 1U)) != 0U)
			return DRIVER_ERROR_INVALID;
	}
	if (config.sram_base + (uintptr_t)(config.size - SUNXI_SID_SRAM_OFFSET) < config.sram_base)
		return DRIVER_ERROR_INVALID;

	hv_switch = (const dt2c_fdt32_t *)dt2c_fdt_getprop(DT2C_FDT_COMPILED_TREE, node, "allwinner,efuse-hv-switch", &hv_switch_length);
	if (hv_switch == NULL) {
		if (hv_switch_length != -DT2C_FDT_ERR_NOTFOUND)
			return DRIVER_ERROR_INVALID;
	} else {
		if (hv_switch_length != (int)sizeof(*hv_switch))
			return DRIVER_ERROR_INVALID;
		config.efuse_hv_switch = (uintptr_t)dt2c_fdt32_to_cpu(hv_switch[0]);
		if (config.efuse_hv_switch == 0U || (config.efuse_hv_switch & (sizeof(uint32_t) - 1U)) != 0U)
			return DRIVER_ERROR_INVALID;
	}

	*sid = config;
	SYTERKIT_DT_TRACE_NODE("sid", node);
	SYTERKIT_DT_TRACE("sid config base=%p size=0x%x sram=%p hv_switch=%p\n", (void *)sid->base, sid->size, (void *)sid->sram_base, (void *)sid->efuse_hv_switch);
	return DRIVER_OK;
}

static inline __attribute__((always_inline)) int sunxi_sid_dt_read_alias(sunxi_sid_t *sid, const char *alias)
{
	int node;

	if (alias == NULL)
		return DRIVER_ERROR_INVALID;
	node = syterkit_dt_alias_node(alias, SUNXI_SID_COMPATIBLE);
	if (node < 0)
		return DRIVER_ERROR_INVALID;
	return sunxi_sid_dt_read_config(sid, node);
}

#endif /* __DT_COMPATIBLE_SID_DT_H__ */
