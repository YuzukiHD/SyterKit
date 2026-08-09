/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __DT_COMPATIBLE_DRAM_DT_H__
#define __DT_COMPATIBLE_DRAM_DT_H__

#include <driver.h>
#include <drivers/dram.h>
#include <dt-compatible/dt-common.h>
#include <dt-compatible/pmu-dt.h>
#include <dt-compatible/rtc-dt.h>

#define SUNXI_DRAM_SUN20IW1_COMPATIBLE "allwinner,sun20iw1-dram"
#define SUNXI_DRAM_SUN252IW1_COMPATIBLE "allwinner,sun252iw1-dram"
#define SUNXI_DRAM_SUN300IW1_COMPATIBLE "allwinner,sun300iw1-dram"
#define SUNXI_DRAM_SUN50IW9_COMPATIBLE "allwinner,sun50iw9-dram"
#define SUNXI_DRAM_SUN50IW10_COMPATIBLE "allwinner,sun50iw10-dram"
#define SUNXI_DRAM_SUN55IW3_COMPATIBLE "allwinner,sun55iw3-dram"
#define SUNXI_DRAM_SUN55IW6_COMPATIBLE "allwinner,sun55iw6-dram"
#define SUNXI_DRAM_SUN60IW2_COMPATIBLE "allwinner,sun60iw2-dram"
#define SUNXI_DRAM_SUN65IW1_COMPATIBLE "allwinner,sun65iw1-dram"
#define SUNXI_DRAM_SUN8IW20_COMPATIBLE "allwinner,sun8iw20-dram"
#define SUNXI_DRAM_SUN8IW21_COMPATIBLE "allwinner,sun8iw21-dram"
#define SUNXI_DRAM_SUN8IW22_COMPATIBLE "allwinner,sun8iw22-dram"

enum sunxi_dram_dt_register_layout {
	SUNXI_DRAM_DT_REGISTERS_NONE = 0,
	SUNXI_DRAM_DT_REGISTERS_D1,
	SUNXI_DRAM_DT_REGISTERS_SUN300IW1,
};

static inline __attribute__((always_inline)) size_t
sunxi_dram_dt_expected_parameter_count(int node) {
	if (dt2c_fdt_node_check_compatible(
			DT2C_FDT_COMPILED_TREE, node,
			SUNXI_DRAM_SUN20IW1_COMPATIBLE) == 0 ||
	    dt2c_fdt_node_check_compatible(
			DT2C_FDT_COMPILED_TREE, node,
			SUNXI_DRAM_SUN252IW1_COMPATIBLE) == 0 ||
	    dt2c_fdt_node_check_compatible(
			DT2C_FDT_COMPILED_TREE, node,
			SUNXI_DRAM_SUN300IW1_COMPATIBLE) == 0 ||
	    dt2c_fdt_node_check_compatible(
			DT2C_FDT_COMPILED_TREE, node,
			SUNXI_DRAM_SUN8IW20_COMPATIBLE) == 0 ||
	    dt2c_fdt_node_check_compatible(
			DT2C_FDT_COMPILED_TREE, node,
			SUNXI_DRAM_SUN8IW21_COMPATIBLE) == 0)
		return 24U;

	if (dt2c_fdt_node_check_compatible(
			DT2C_FDT_COMPILED_TREE, node,
			SUNXI_DRAM_SUN50IW9_COMPATIBLE) == 0 ||
	    dt2c_fdt_node_check_compatible(
			DT2C_FDT_COMPILED_TREE, node,
			SUNXI_DRAM_SUN50IW10_COMPATIBLE) == 0 ||
	    dt2c_fdt_node_check_compatible(
			DT2C_FDT_COMPILED_TREE, node,
			SUNXI_DRAM_SUN55IW3_COMPATIBLE) == 0 ||
	    dt2c_fdt_node_check_compatible(
			DT2C_FDT_COMPILED_TREE, node,
			SUNXI_DRAM_SUN55IW6_COMPATIBLE) == 0 ||
	    dt2c_fdt_node_check_compatible(
			DT2C_FDT_COMPILED_TREE, node,
			SUNXI_DRAM_SUN60IW2_COMPATIBLE) == 0 ||
	    dt2c_fdt_node_check_compatible(
			DT2C_FDT_COMPILED_TREE, node,
			SUNXI_DRAM_SUN65IW1_COMPATIBLE) == 0 ||
	    dt2c_fdt_node_check_compatible(
			DT2C_FDT_COMPILED_TREE, node,
			SUNXI_DRAM_SUN8IW22_COMPATIBLE) == 0)
		return 32U;

	return 0U;
}

static inline __attribute__((always_inline)) unsigned int
sunxi_dram_dt_required_pmus(int node) {
	if (dt2c_fdt_node_check_compatible(
			DT2C_FDT_COMPILED_TREE, node,
			SUNXI_DRAM_SUN65IW1_COMPATIBLE) == 0)
		return 3U;
	if (dt2c_fdt_node_check_compatible(
			DT2C_FDT_COMPILED_TREE, node,
			SUNXI_DRAM_SUN50IW10_COMPATIBLE) == 0 ||
	    dt2c_fdt_node_check_compatible(
			DT2C_FDT_COMPILED_TREE, node,
			SUNXI_DRAM_SUN60IW2_COMPATIBLE) == 0)
		return 1U;
	return 0U;
}

static inline __attribute__((always_inline)) enum sunxi_dram_dt_register_layout
sunxi_dram_dt_register_layout(int node) {
	if (dt2c_fdt_node_check_compatible(
			DT2C_FDT_COMPILED_TREE, node,
			SUNXI_DRAM_SUN20IW1_COMPATIBLE) == 0 ||
	    dt2c_fdt_node_check_compatible(
			DT2C_FDT_COMPILED_TREE, node,
			SUNXI_DRAM_SUN8IW20_COMPATIBLE) == 0 ||
	    dt2c_fdt_node_check_compatible(
			DT2C_FDT_COMPILED_TREE, node,
			SUNXI_DRAM_SUN8IW21_COMPATIBLE) == 0)
		return SUNXI_DRAM_DT_REGISTERS_D1;

	if (dt2c_fdt_node_check_compatible(
			DT2C_FDT_COMPILED_TREE, node,
			SUNXI_DRAM_SUN300IW1_COMPATIBLE) == 0)
		return SUNXI_DRAM_DT_REGISTERS_SUN300IW1;

	return SUNXI_DRAM_DT_REGISTERS_NONE;
}

static inline __attribute__((always_inline)) bool
sunxi_dram_dt_register(const dt2c_fdt32_t *cells, size_t index,
		       size_t minimum_size, sunxi_dram_register_t *reg) {
	uint32_t base = dt2c_fdt32_to_cpu(cells[index * 2U]);
	uint32_t size = dt2c_fdt32_to_cpu(cells[index * 2U + 1U]);

	if (base == 0U || (base & 3U) != 0U || size < minimum_size ||
	    (size & 3U) != 0U || base + size < base)
		return false;
	reg->base = (uintptr_t) base;
	reg->size = (size_t) size;
	return true;
}

static inline __attribute__((always_inline)) int
sunxi_dram_dt_registers(int node, sunxi_dram_registers_t *registers) {
	const dt2c_fdt32_t *cells;
	enum sunxi_dram_dt_register_layout layout;
	int length;

	layout = sunxi_dram_dt_register_layout(node);
	cells = (const dt2c_fdt32_t *) dt2c_fdt_getprop(
			DT2C_FDT_COMPILED_TREE, node, "reg", &length);
	if (layout == SUNXI_DRAM_DT_REGISTERS_NONE) {
		if (cells == NULL && length == -DT2C_FDT_ERR_NOTFOUND) {
			*registers = (sunxi_dram_registers_t) {0};
			return DRIVER_OK;
		}
		return DRIVER_ERROR_INVALID;
	}
	if (cells == NULL || length != 14 * (int) sizeof(*cells))
		return DRIVER_ERROR_INVALID;

	*registers = (sunxi_dram_registers_t) {0};
	if (layout == SUNXI_DRAM_DT_REGISTERS_D1) {
		if (!sunxi_dram_dt_register(cells, 0U, 0x810U,
					    &registers->ccu) ||
		    !sunxi_dram_dt_register(cells, 1U, 0x510U,
					    &registers->mctl_com) ||
		    !sunxi_dram_dt_register(cells, 2U, 0x500U,
					    &registers->mctl_phy) ||
		    !sunxi_dram_dt_register(cells, 3U, 0x170U,
					    &registers->sysctrl) ||
		    !sunxi_dram_dt_register(cells, 4U, 0x22cU,
					    &registers->sid) ||
		    !sunxi_dram_dt_register(cells, 5U, 0x1d8U,
					    &registers->r_cpucfg) ||
		    !sunxi_dram_dt_register(cells, 6U, 0x258U,
					    &registers->r_prcm))
			return DRIVER_ERROR_INVALID;
		return DRIVER_OK;
	}

	if (!sunxi_dram_dt_register(cells, 0U, 0x100U,
				    &registers->ccu) ||
	    !sunxi_dram_dt_register(cells, 1U, 0x84U,
				    &registers->aon_ccu) ||
	    !sunxi_dram_dt_register(cells, 2U, 0x510U,
				    &registers->mctl_com) ||
	    !sunxi_dram_dt_register(cells, 3U, 0x500U,
				    &registers->mctl_phy) ||
	    !sunxi_dram_dt_register(cells, 4U, 0x170U,
				    &registers->sysctrl) ||
	    !sunxi_dram_dt_register(cells, 5U, 0x1c4U,
				    &registers->r_prcm) ||
	    !sunxi_dram_dt_register(cells, 6U, 0x40U,
				    &registers->pmu_rtc))
		return DRIVER_ERROR_INVALID;
	return DRIVER_OK;
}

static inline __attribute__((always_inline)) int
sunxi_dram_dt_pmu(int node, const char *property, axp_pmu_t *supplied,
		    axp_pmu_t **resolved) {
	const dt2c_fdt32_t *phandle;
	const dt2c_fdt32_t *reg;
	axp_pmu_type_t type;
	uint32_t address;
	uint32_t fallback_address;
	int length;
	int pmu_node;
	int reg_length;

	phandle = (const dt2c_fdt32_t *) dt2c_fdt_getprop(
			DT2C_FDT_COMPILED_TREE, node, property, &length);
	if (phandle == NULL && length == -DT2C_FDT_ERR_NOTFOUND) {
		if (supplied != NULL)
			return DRIVER_ERROR_INVALID;
		*resolved = NULL;
		return DRIVER_OK;
	}
	if (phandle == NULL || length != (int) sizeof(*phandle) ||
	    supplied == NULL)
		return DRIVER_ERROR_INVALID;
	pmu_node = dt2c_fdt_node_offset_by_phandle(
			DT2C_FDT_COMPILED_TREE, dt2c_fdt32_to_cpu(phandle[0]));
	if (pmu_node < 0 || !syterkit_dt_node_available(pmu_node) ||
	    !sunxi_pmu_dt_type(pmu_node, &type) || supplied->type != type ||
	    sunxi_pmu_dt_parent_i2c(pmu_node, supplied->i2c) != supplied->i2c)
		return DRIVER_ERROR_INVALID;
	reg = (const dt2c_fdt32_t *) dt2c_fdt_getprop(
			DT2C_FDT_COMPILED_TREE, pmu_node, "reg", &reg_length);
	if (reg == NULL ||
	    (reg_length != (int) sizeof(*reg) &&
	     reg_length != 2 * (int) sizeof(*reg)))
		return DRIVER_ERROR_INVALID;
	address = dt2c_fdt32_to_cpu(reg[0]);
	fallback_address = reg_length == 2 * (int) sizeof(*reg) ?
			dt2c_fdt32_to_cpu(reg[1]) : 0U;
	if (address == 0U || address > 0x7fU || fallback_address > 0x7fU ||
	    fallback_address == address || supplied->address != address ||
	    supplied->fallback_address != fallback_address)
		return DRIVER_ERROR_INVALID;
	*resolved = supplied;
	return DRIVER_OK;
}

static inline __attribute__((always_inline)) int
sunxi_dram_dt_rtc(int node, sunxi_rtc_t *rtc) {
	const dt2c_fdt32_t *phandle;
	bool required;
	int length;
	int rtc_node;

	required = dt2c_fdt_node_check_compatible(
			DT2C_FDT_COMPILED_TREE, node,
			SUNXI_DRAM_SUN50IW9_COMPATIBLE) == 0 ||
		   dt2c_fdt_node_check_compatible(
			DT2C_FDT_COMPILED_TREE, node,
			SUNXI_DRAM_SUN8IW21_COMPATIBLE) == 0;
	phandle = (const dt2c_fdt32_t *) dt2c_fdt_getprop(
			DT2C_FDT_COMPILED_TREE, node, "allwinner,rtc", &length);
	if (phandle == NULL && length == -DT2C_FDT_ERR_NOTFOUND) {
		*rtc = (sunxi_rtc_t) {0};
		return required ? DRIVER_ERROR_INVALID : DRIVER_OK;
	}
	if (phandle == NULL || length != (int) sizeof(*phandle))
		return DRIVER_ERROR_INVALID;
	rtc_node = dt2c_fdt_node_offset_by_phandle(
			DT2C_FDT_COMPILED_TREE, dt2c_fdt32_to_cpu(phandle[0]));
	if (sunxi_rtc_dt_read_config(rtc, rtc_node) != DRIVER_OK)
		return DRIVER_ERROR_INVALID;
	if (dt2c_fdt_node_check_compatible(
			DT2C_FDT_COMPILED_TREE, node,
			SUNXI_DRAM_SUN8IW21_COMPATIBLE) == 0 &&
	    rtc->data_size < 0xfcU)
		return DRIVER_ERROR_INVALID;
	return DRIVER_OK;
}

static inline __attribute__((always_inline)) int
sunxi_dram_dt_init_code(int node, uintptr_t *base, size_t *size) {
	const dt2c_fdt32_t *phandle;
	const dt2c_fdt32_t *reg;
	bool required;
	uint32_t address;
	uint32_t region_size;
	int length;
	int region_node;

	required = dt2c_fdt_node_check_compatible(
			DT2C_FDT_COMPILED_TREE, node,
			SUNXI_DRAM_SUN50IW9_COMPATIBLE) == 0;
	phandle = (const dt2c_fdt32_t *) dt2c_fdt_getprop(
			DT2C_FDT_COMPILED_TREE, node, "memory-region", &length);
	if (phandle == NULL && length == -DT2C_FDT_ERR_NOTFOUND) {
		*base = 0U;
		*size = 0U;
		return required ? DRIVER_ERROR_INVALID : DRIVER_OK;
	}
	if (phandle == NULL || length != (int) sizeof(*phandle))
		return DRIVER_ERROR_INVALID;
	region_node = dt2c_fdt_node_offset_by_phandle(
			DT2C_FDT_COMPILED_TREE, dt2c_fdt32_to_cpu(phandle[0]));
	if (region_node < 0 || !syterkit_dt_node_available(region_node))
		return DRIVER_ERROR_INVALID;
	reg = syterkit_dt_cells(region_node, "reg", 2);
	if (reg == NULL)
		return DRIVER_ERROR_INVALID;
	address = dt2c_fdt32_to_cpu(reg[0]);
	region_size = dt2c_fdt32_to_cpu(reg[1]);
	if (address == 0U || (address & 3U) != 0U || region_size < 4U ||
	    (region_size & 3U) != 0U || address + region_size < address)
		return DRIVER_ERROR_INVALID;
	*base = (uintptr_t) address;
	*size = (size_t) region_size;
	return DRIVER_OK;
}

static inline __attribute__((always_inline)) bool
sunxi_dram_dt_memory_required(int node) {
	return dt2c_fdt_node_check_compatible(
			DT2C_FDT_COMPILED_TREE, node,
			SUNXI_DRAM_SUN20IW1_COMPATIBLE) == 0 ||
	       dt2c_fdt_node_check_compatible(
			DT2C_FDT_COMPILED_TREE, node,
			SUNXI_DRAM_SUN300IW1_COMPATIBLE) == 0 ||
	       dt2c_fdt_node_check_compatible(
			DT2C_FDT_COMPILED_TREE, node,
			SUNXI_DRAM_SUN8IW20_COMPATIBLE) == 0 ||
	       dt2c_fdt_node_check_compatible(
			DT2C_FDT_COMPILED_TREE, node,
			SUNXI_DRAM_SUN8IW21_COMPATIBLE) == 0;
}

static inline __attribute__((always_inline)) int
sunxi_dram_dt_memory(int node, uintptr_t *base, size_t *size) {
	const dt2c_fdt32_t *phandle;
	const dt2c_fdt32_t *reg;
	const char *device_type;
	uint32_t address;
	uint32_t region_size;
	int length;
	int memory_node;

	phandle = (const dt2c_fdt32_t *) dt2c_fdt_getprop(
			DT2C_FDT_COMPILED_TREE, node,
			"allwinner,dram-memory", &length);
	if (phandle == NULL && length == -DT2C_FDT_ERR_NOTFOUND) {
		*base = 0U;
		*size = 0U;
		return sunxi_dram_dt_memory_required(node) ?
			DRIVER_ERROR_INVALID : DRIVER_OK;
	}
	if (phandle == NULL || length != (int) sizeof(*phandle))
		return DRIVER_ERROR_INVALID;
	memory_node = dt2c_fdt_node_offset_by_phandle(
			DT2C_FDT_COMPILED_TREE, dt2c_fdt32_to_cpu(phandle[0]));
	if (memory_node < 0 || !syterkit_dt_node_available(memory_node))
		return DRIVER_ERROR_INVALID;
	device_type = (const char *) dt2c_fdt_getprop(
			DT2C_FDT_COMPILED_TREE, memory_node,
			"device_type", &length);
	if (!syterkit_dt_string_equal(device_type, length, "memory", 6U))
		return DRIVER_ERROR_INVALID;
	reg = syterkit_dt_cells(memory_node, "reg", 2U);
	if (reg == NULL)
		return DRIVER_ERROR_INVALID;
	address = dt2c_fdt32_to_cpu(reg[0]);
	region_size = dt2c_fdt32_to_cpu(reg[1]);
	if ((address & 3U) != 0U || region_size < 4U ||
	    (region_size & 3U) != 0U ||
	    (uint64_t) address + region_size > 0x100000000ULL)
		return DRIVER_ERROR_INVALID;
	*base = (uintptr_t) address;
	*size = (size_t) region_size;
	return DRIVER_OK;
}

static inline __attribute__((always_inline)) int
sunxi_dram_dt_read_config(sunxi_dram_t *dram, int node,
			    axp_pmu_t *primary, axp_pmu_t *secondary) {
	const dt2c_fdt32_t *parameters;
	axp_pmu_t *resolved_primary;
	axp_pmu_t *resolved_secondary;
	sunxi_rtc_t resolved_rtc;
	sunxi_dram_registers_t resolved_registers;
	uintptr_t memory_base;
	size_t memory_size;
	uintptr_t init_code_base;
	size_t init_code_size;
	size_t count;
	size_t expected_count;
	size_t index;
	unsigned int required_pmus;
	int length;

	if (dram == NULL || node < 0 || !syterkit_dt_node_available(node))
		return DRIVER_ERROR_INVALID;
	expected_count = sunxi_dram_dt_expected_parameter_count(node);
	required_pmus = sunxi_dram_dt_required_pmus(node);
	parameters = (const dt2c_fdt32_t *) dt2c_fdt_getprop(
			DT2C_FDT_COMPILED_TREE, node,
			"allwinner,dram-parameters", &length);
	if (expected_count == 0U || parameters == NULL ||
	    length != (int) (expected_count * sizeof(*parameters)) ||
	    sunxi_dram_dt_pmu(node, "allwinner,primary-pmu", primary,
			       &resolved_primary) != DRIVER_OK ||
	    sunxi_dram_dt_pmu(node, "allwinner,secondary-pmu", secondary,
			       &resolved_secondary) != DRIVER_OK ||
	    sunxi_dram_dt_registers(node, &resolved_registers) != DRIVER_OK ||
	    sunxi_dram_dt_rtc(node, &resolved_rtc) != DRIVER_OK ||
	    sunxi_dram_dt_memory(node, &memory_base, &memory_size) != DRIVER_OK ||
	    sunxi_dram_dt_init_code(node, &init_code_base,
				    &init_code_size) != DRIVER_OK ||
	    ((required_pmus & 1U) != 0U && resolved_primary == NULL) ||
	    ((required_pmus & 2U) != 0U && resolved_secondary == NULL))
		return DRIVER_ERROR_INVALID;

	count = (size_t) length / sizeof(*parameters);
	for (index = 0; index < count; ++index)
		dram->parameters[index] = dt2c_fdt32_to_cpu(parameters[index]);
	for (; index < SUNXI_DRAM_MAX_PARAM_WORDS; ++index)
		dram->parameters[index] = 0U;
	dram->parameter_count = count;
	dram->dt_node = node;
	dram->size = 0U;
	dram->memory_base = memory_base;
	dram->memory_size = memory_size;
	dram->registers = resolved_registers;
	dram->init_code_base = init_code_base;
	dram->init_code_size = init_code_size;
	dram->primary_pmu = resolved_primary;
	dram->secondary_pmu = resolved_secondary;
	dram->rtc = resolved_rtc;
	return DRIVER_OK;
}

static inline __attribute__((always_inline)) int
sunxi_dram_dt_read_alias(sunxi_dram_t *dram, const char *alias,
			   axp_pmu_t *primary, axp_pmu_t *secondary) {
	int node;

	if (alias == NULL)
		return DRIVER_ERROR_INVALID;
	node = syterkit_dt_alias_node(alias, NULL);
	if (node < 0)
		return DRIVER_ERROR_INVALID;
	return sunxi_dram_dt_read_config(dram, node, primary, secondary);
}

#endif /* __DT_COMPATIBLE_DRAM_DT_H__ */
