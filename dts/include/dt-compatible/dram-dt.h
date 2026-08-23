/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __DT_COMPATIBLE_DRAM_DT_H__
#define __DT_COMPATIBLE_DRAM_DT_H__

#include <driver.h>
#include <drivers/dram/dram.h>
#include <dt-compatible/dt-common.h>

#define SUNXI_DRAM_COMPATIBLE "allwinner,sunxi-dram"

#define SUNXI_DRAM_DT_PARAMETER_COUNT 32U

static inline __attribute__((always_inline)) int
sunxi_dram_dt_read_config(sunxi_dram_t *dram, int node) {
	const dt2c_fdt32_t *parameters;
	sunxi_dram_t config;
	size_t count;
	size_t index;
	int length;

	if (dram == NULL || node < 0 || !syterkit_dt_node_available(node) ||
		dt2c_fdt_node_check_compatible(DT2C_FDT_COMPILED_TREE, node,
									   SUNXI_DRAM_COMPATIBLE) != 0)
		return DRIVER_ERROR_INVALID;

	parameters = (const dt2c_fdt32_t *) dt2c_fdt_getprop(
			DT2C_FDT_COMPILED_TREE, node, "allwinner,dram-parameters",
			&length);

	if (parameters == NULL ||
		length != (int) (SUNXI_DRAM_DT_PARAMETER_COUNT * sizeof(*parameters)))
		return DRIVER_ERROR_INVALID;

	count = SUNXI_DRAM_DT_PARAMETER_COUNT;

	/* Keep platform-populated resources; DT contributes parameters only. */
	config = *dram;
	for (index = 0U; index < count; ++index)
		config.parameters[index] = dt2c_fdt32_to_cpu(parameters[index]);

	for (; index < SUNXI_DRAM_MAX_PARAM_WORDS; ++index)
		config.parameters[index] = 0U;

	config.parameter_count = count;
	config.dt_node = node;
	config.size = 0U;
	*dram = config;

	SYTERKIT_DT_TRACE_NODE("dram", node);
	SYTERKIT_DT_TRACE("dram parameters=%lu\n",
					  (unsigned long) dram->parameter_count);
	return DRIVER_OK;
}

static inline __attribute__((always_inline)) int
sunxi_dram_dt_read_alias(sunxi_dram_t *dram, const char *alias) {
	if (alias == NULL)
		return DRIVER_ERROR_INVALID;
	return sunxi_dram_dt_read_config(
			dram, syterkit_dt_alias_node(alias, SUNXI_DRAM_COMPATIBLE));
}

#endif /* __DT_COMPATIBLE_DRAM_DT_H__ */
