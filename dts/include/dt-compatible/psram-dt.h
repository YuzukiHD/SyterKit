/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __DT_COMPATIBLE_PSRAM_DT_H__
#define __DT_COMPATIBLE_PSRAM_DT_H__

#include <driver.h>
#include <drivers/psram/psram.h>
#include <dt-compatible/dt-common.h>

#define SUNXI_PSRAM_COMPATIBLE "allwinner,sunxi-psram"

#define SUNXI_PSRAM_DT_PARAMETER_COUNT 32U

static inline __attribute__((always_inline)) int sunxi_psram_dt_read_config(sunxi_psram_t *psram, int node)
{
	const dt2c_fdt32_t *psram_base;
	const dt2c_fdt32_t *parameters;
	sunxi_psram_t config;
	size_t count;
	size_t index;
	int psram_base_length;
	int length;

	if (psram == NULL || node < 0 || !syterkit_dt_node_available(node) || dt2c_fdt_node_check_compatible(DT2C_FDT_COMPILED_TREE, node, SUNXI_PSRAM_COMPATIBLE) != 0)
		return DRIVER_ERROR_INVALID;

	parameters = (const dt2c_fdt32_t *)dt2c_fdt_getprop(DT2C_FDT_COMPILED_TREE, node, "allwinner,psram-parameters", &length);

	/* Accept 1..32 words; the init library auto-fills the rest. */
	if (parameters == NULL || length < (int)sizeof(*parameters))
		return DRIVER_ERROR_INVALID;

	count = (size_t)length / sizeof(*parameters);
	if (count > SUNXI_PSRAM_DT_PARAMETER_COUNT)
		count = SUNXI_PSRAM_DT_PARAMETER_COUNT;

	/* Keep platform-populated resources; DT overrides resources it describes. */
	config = *psram;
	psram_base = (const dt2c_fdt32_t *)dt2c_fdt_getprop(DT2C_FDT_COMPILED_TREE, node, "allwinner,psram-base", &psram_base_length);
	if (psram_base != NULL) {
		if (psram_base_length != (int)sizeof(*psram_base) || dt2c_fdt32_to_cpu(psram_base[0]) == 0U)
			return DRIVER_ERROR_INVALID;
		config.memory_base = (uintptr_t)dt2c_fdt32_to_cpu(psram_base[0]);
	}

	for (index = 0U; index < count; ++index)
		config.parameters[index] = dt2c_fdt32_to_cpu(parameters[index]);

	for (; index < SUNXI_PSRAM_MAX_PARAM_WORDS; ++index)
		config.parameters[index] = 0U;

	config.parameter_count = count;
	config.dt_node = node;
	config.size = 0U;
	*psram = config;

	SYTERKIT_DT_TRACE_NODE("psram", node);
	SYTERKIT_DT_TRACE("psram base=0x%08x\n", (uint32_t)psram->memory_base);
	SYTERKIT_DT_TRACE("psram parameters=%lu\n", (unsigned long)psram->parameter_count);
	return DRIVER_OK;
}

static inline __attribute__((always_inline)) int sunxi_psram_dt_read_alias(sunxi_psram_t *psram, const char *alias)
{
	if (alias == NULL)
		return DRIVER_ERROR_INVALID;
	return sunxi_psram_dt_read_config(psram, syterkit_dt_alias_node(alias, SUNXI_PSRAM_COMPATIBLE));
}

#endif /* __DT_COMPATIBLE_PSRAM_DT_H__ */
