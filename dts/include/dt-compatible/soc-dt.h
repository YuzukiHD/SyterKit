/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __DT_COMPATIBLE_SOC_DT_H__
#define __DT_COMPATIBLE_SOC_DT_H__

#include <driver.h>
#include <drivers/soc/soc.h>
#include <dt-compatible/dt-common.h>

/**
 * @brief Read the SoC identification register window from a device-tree node.
 *
 * Resolves the allwinner,soc node's reg property into the register window
 * base and size.
 *
 * @param[out] soc Descriptor to populate.
 * @param[in] node Device-tree node offset.
 * @return DRIVER_OK on success, DRIVER_ERROR_INVALID otherwise.
 */
static inline __attribute__((always_inline)) int sunxi_soc_dt_read_config(sunxi_soc_t *soc, int node)
{
	const dt2c_fdt32_t *reg;
	sunxi_soc_t config = { 0 };

	if (soc == NULL || node < 0 || !syterkit_dt_node_available(node) ||
	    dt2c_fdt_node_check_compatible(DT2C_FDT_COMPILED_TREE, node, SUNXI_SOC_COMPATIBLE) != 0)
		return DRIVER_ERROR_INVALID;

	reg = syterkit_dt_cells(node, "reg", 2);
	if (reg == NULL)
		return DRIVER_ERROR_INVALID;

	config.dt_node = node;
	config.base = (uintptr_t)dt2c_fdt32_to_cpu(reg[0]);
	config.size = (size_t)dt2c_fdt32_to_cpu(reg[1]);
	if (config.base == 0U || config.size == 0U || config.base + (uintptr_t)config.size < config.base)
		return DRIVER_ERROR_INVALID;

	*soc = config;
	SYTERKIT_DT_TRACE_NODE("soc", node);
	SYTERKIT_DT_TRACE("soc config base=%p size=0x%x\n", (void *)soc->base, soc->size);
	return DRIVER_OK;
}

/**
 * @brief Read the SoC identification register window via a device-tree alias.
 *
 * @param[out] soc Descriptor to populate.
 * @param[in] alias Device-tree alias name.
 * @return DRIVER_OK on success, DRIVER_ERROR_INVALID otherwise.
 */
static inline __attribute__((always_inline)) int sunxi_soc_dt_read_alias(sunxi_soc_t *soc, const char *alias)
{
	int node;

	if (alias == NULL)
		return DRIVER_ERROR_INVALID;
	node = syterkit_dt_alias_node(alias, SUNXI_SOC_COMPATIBLE);
	if (node < 0)
		return DRIVER_ERROR_INVALID;
	return sunxi_soc_dt_read_config(soc, node);
}

#endif /* __DT_COMPATIBLE_SOC_DT_H__ */
