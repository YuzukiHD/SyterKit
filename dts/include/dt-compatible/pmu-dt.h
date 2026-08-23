/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __DT_COMPATIBLE_PMU_DT_H__
#define __DT_COMPATIBLE_PMU_DT_H__

#include <driver.h>
#include <drivers/pmu/axp.h>
#include <dt-compatible/dt-common.h>

#define AXP1530_COMPATIBLE "x-powers,axp1530"
#define AXP2101_COMPATIBLE "x-powers,axp2101"
#define AXP2202_COMPATIBLE "x-powers,axp2202"
#define AXP333_COMPATIBLE "x-powers,axp333"
#define AXP8191_COMPATIBLE "x-powers,axp8191"

static inline __attribute__((always_inline)) bool
sunxi_pmu_dt_type(int node, axp_pmu_type_t *type) {
	if (dt2c_fdt_node_check_compatible(DT2C_FDT_COMPILED_TREE, node,
					   AXP1530_COMPATIBLE) == 0)
		*type = AXP_PMU_AXP1530;
	else if (dt2c_fdt_node_check_compatible(DT2C_FDT_COMPILED_TREE, node,
						AXP2101_COMPATIBLE) == 0)
		*type = AXP_PMU_AXP2101;
	else if (dt2c_fdt_node_check_compatible(DT2C_FDT_COMPILED_TREE, node,
						AXP2202_COMPATIBLE) == 0)
		*type = AXP_PMU_AXP2202;
	else if (dt2c_fdt_node_check_compatible(DT2C_FDT_COMPILED_TREE, node,
						AXP333_COMPATIBLE) == 0)
		*type = AXP_PMU_AXP333;
	else if (dt2c_fdt_node_check_compatible(DT2C_FDT_COMPILED_TREE, node,
						AXP8191_COMPATIBLE) == 0)
		*type = AXP_PMU_AXP8191;
	else
		return false;
	return true;
}

static inline __attribute__((always_inline)) sunxi_i2c_t *
sunxi_pmu_dt_parent_i2c(int node, sunxi_i2c_t *i2c) {
	int parent;

	parent = dt2c_fdt_parent_offset(DT2C_FDT_COMPILED_TREE, node);
	if (i2c == NULL || parent < 0 || i2c->dt_node != parent ||
	    !syterkit_dt_node_available(parent) ||
	    dt2c_fdt_node_check_compatible(DT2C_FDT_COMPILED_TREE, parent,
					   SUNXI_I2C_COMPATIBLE) != 0)
		return NULL;
	return i2c;
}

static inline __attribute__((always_inline)) int
sunxi_pmu_dt_read_config(axp_pmu_t *pmu, int node, sunxi_i2c_t *i2c) {
	const dt2c_fdt32_t *reg;
	int length;
	axp_pmu_t config = {0};
	uint32_t address;
	uint32_t fallback_address;

	if (pmu == NULL || node < 0 || !syterkit_dt_node_available(node) ||
	    !sunxi_pmu_dt_type(node, &config.type))
		return DRIVER_ERROR_INVALID;
	config.i2c = sunxi_pmu_dt_parent_i2c(node, i2c);
	if (config.i2c == NULL)
		return DRIVER_ERROR_INVALID;
	reg = (const dt2c_fdt32_t *) dt2c_fdt_getprop(
			DT2C_FDT_COMPILED_TREE, node, "reg", &length);
	if (reg == NULL || (length != (int) sizeof(*reg) &&
			    length != (int) (2U * sizeof(*reg))))
		return DRIVER_ERROR_INVALID;
	address = dt2c_fdt32_to_cpu(reg[0]);
	fallback_address = length == (int) (2U * sizeof(*reg)) ?
			dt2c_fdt32_to_cpu(reg[1]) : 0U;
	if (address == 0U || address > 0x7fU || fallback_address > 0x7fU ||
	    fallback_address == address)
		return DRIVER_ERROR_INVALID;
	config.address = (uint8_t) address;
	config.fallback_address = (uint8_t) fallback_address;
	*pmu = config;
	SYTERKIT_DT_TRACE_NODE("pmu", node);
	SYTERKIT_DT_TRACE("pmu config type=%u address=0x%02x fallback=0x%02x i2c=%p\n",
			 (unsigned int) pmu->type, pmu->address,
			 pmu->fallback_address, (void *) pmu->i2c);
	return DRIVER_OK;
}

static inline __attribute__((always_inline)) int
sunxi_pmu_dt_read_alias(axp_pmu_t *pmu, const char *alias,
				sunxi_i2c_t *i2c) {
	return sunxi_pmu_dt_read_config(
			pmu, syterkit_dt_alias_node(alias, NULL), i2c);
}

#endif /* __DT_COMPATIBLE_PMU_DT_H__ */
