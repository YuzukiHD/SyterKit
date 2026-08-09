/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __DT_COMPATIBLE_CCU_DT_H__
#define __DT_COMPATIBLE_CCU_DT_H__

#include <stdbool.h>

#include <driver.h>
#include <drivers/clk.h>
#include <dt-compatible/dt-common.h>

#define SUNXI_CCU_SUN20IW1_COMPATIBLE "allwinner,sun20iw1-ccu"
#define SUNXI_CCU_SUN252IW1_COMPATIBLE "allwinner,sun252iw1-ccu"
#define SUNXI_CCU_SUN300IW1_COMPATIBLE "allwinner,sun300iw1-ccu"
#define SUNXI_CCU_SUN50IW9_COMPATIBLE "allwinner,sun50iw9-ccu"
#define SUNXI_CCU_SUN50IW10_COMPATIBLE "allwinner,sun50iw10-ccu"
#define SUNXI_CCU_SUN55IW3_COMPATIBLE "allwinner,sun55iw3-ccu"
#define SUNXI_CCU_SUN55IW6_COMPATIBLE "allwinner,sun55iw6-ccu"
#define SUNXI_CCU_SUN60IW2_COMPATIBLE "allwinner,sun60iw2-ccu"
#define SUNXI_CCU_SUN65IW1_COMPATIBLE "allwinner,sun65iw1-ccu"
#define SUNXI_CCU_SUN8IW20_COMPATIBLE "allwinner,sun8iw20-ccu"
#define SUNXI_CCU_SUN8IW21_COMPATIBLE "allwinner,sun8iw21-ccu"
#define SUNXI_CCU_SUN8IW22_COMPATIBLE "allwinner,sun8iw22-ccu"

enum sunxi_ccu_dt_layout {
	SUNXI_CCU_DT_LAYOUT_INVALID = 0,
	SUNXI_CCU_DT_LAYOUT_SINGLE,
	SUNXI_CCU_DT_LAYOUT_APP_AON,
	SUNXI_CCU_DT_LAYOUT_R_PRCM_IOMMU,
	SUNXI_CCU_DT_LAYOUT_R_PRCM_SYSCTRL_IOMMU,
	SUNXI_CCU_DT_LAYOUT_CPU_SYS_CFG_R_PRCM_IOMMU,
	SUNXI_CCU_DT_LAYOUT_CPU_PLL,
	SUNXI_CCU_DT_LAYOUT_CPU_PLL_RTC,
};

static inline __attribute__((always_inline)) bool
sunxi_ccu_dt_is_compatible(int node, const char *compatible) {
	return dt2c_fdt_node_check_compatible(
			DT2C_FDT_COMPILED_TREE, node, compatible) == 0;
}

static inline __attribute__((always_inline)) enum sunxi_ccu_dt_layout
sunxi_ccu_dt_layout(int node) {
	if (sunxi_ccu_dt_is_compatible(
			node, SUNXI_CCU_SUN300IW1_COMPATIBLE))
		return SUNXI_CCU_DT_LAYOUT_APP_AON;
	if (sunxi_ccu_dt_is_compatible(
			node, SUNXI_CCU_SUN50IW9_COMPATIBLE))
		return SUNXI_CCU_DT_LAYOUT_R_PRCM_IOMMU;
	if (sunxi_ccu_dt_is_compatible(
			node, SUNXI_CCU_SUN50IW10_COMPATIBLE))
		return SUNXI_CCU_DT_LAYOUT_R_PRCM_SYSCTRL_IOMMU;
	if (sunxi_ccu_dt_is_compatible(
			node, SUNXI_CCU_SUN55IW3_COMPATIBLE))
		return SUNXI_CCU_DT_LAYOUT_CPU_SYS_CFG_R_PRCM_IOMMU;
	if (sunxi_ccu_dt_is_compatible(
			node, SUNXI_CCU_SUN55IW6_COMPATIBLE) ||
	    sunxi_ccu_dt_is_compatible(
			node, SUNXI_CCU_SUN8IW22_COMPATIBLE))
		return SUNXI_CCU_DT_LAYOUT_CPU_PLL;
	if (sunxi_ccu_dt_is_compatible(
			node, SUNXI_CCU_SUN60IW2_COMPATIBLE))
		return SUNXI_CCU_DT_LAYOUT_CPU_PLL_RTC;
	if (sunxi_ccu_dt_is_compatible(
			node, SUNXI_CCU_SUN20IW1_COMPATIBLE) ||
	    sunxi_ccu_dt_is_compatible(
			node, SUNXI_CCU_SUN252IW1_COMPATIBLE) ||
	    sunxi_ccu_dt_is_compatible(
			node, SUNXI_CCU_SUN65IW1_COMPATIBLE) ||
	    sunxi_ccu_dt_is_compatible(
			node, SUNXI_CCU_SUN8IW20_COMPATIBLE) ||
	    sunxi_ccu_dt_is_compatible(
			node, SUNXI_CCU_SUN8IW21_COMPATIBLE))
		return SUNXI_CCU_DT_LAYOUT_SINGLE;
	return SUNXI_CCU_DT_LAYOUT_INVALID;
}

static inline __attribute__((always_inline)) size_t
sunxi_ccu_dt_register_count(enum sunxi_ccu_dt_layout layout) {
	switch (layout) {
		case SUNXI_CCU_DT_LAYOUT_SINGLE:
			return 1U;
		case SUNXI_CCU_DT_LAYOUT_APP_AON:
		case SUNXI_CCU_DT_LAYOUT_CPU_PLL:
			return 2U;
		case SUNXI_CCU_DT_LAYOUT_R_PRCM_IOMMU:
		case SUNXI_CCU_DT_LAYOUT_CPU_PLL_RTC:
			return 3U;
		case SUNXI_CCU_DT_LAYOUT_R_PRCM_SYSCTRL_IOMMU:
		case SUNXI_CCU_DT_LAYOUT_CPU_SYS_CFG_R_PRCM_IOMMU:
			return 4U;
		default:
			return 0U;
	}
}

static inline __attribute__((always_inline)) size_t
sunxi_ccu_dt_main_minimum_size(int node) {
	if (sunxi_ccu_dt_is_compatible(
			node, SUNXI_CCU_SUN20IW1_COMPATIBLE) ||
	    sunxi_ccu_dt_is_compatible(
			node, SUNXI_CCU_SUN252IW1_COMPATIBLE))
		return 0xd04U;
	if (sunxi_ccu_dt_is_compatible(
			node, SUNXI_CCU_SUN300IW1_COMPATIBLE))
		return 0x100U;
	if (sunxi_ccu_dt_is_compatible(
			node, SUNXI_CCU_SUN50IW9_COMPATIBLE))
		return 0x9f0U;
	if (sunxi_ccu_dt_is_compatible(
			node, SUNXI_CCU_SUN50IW10_COMPATIBLE) ||
	    sunxi_ccu_dt_is_compatible(
			node, SUNXI_CCU_SUN55IW3_COMPATIBLE))
		return 0x7c0U;
	if (sunxi_ccu_dt_is_compatible(
			node, SUNXI_CCU_SUN55IW6_COMPATIBLE))
		return 0x58cU;
	if (sunxi_ccu_dt_is_compatible(
			node, SUNXI_CCU_SUN60IW2_COMPATIBLE))
		return 0x584U;
	if (sunxi_ccu_dt_is_compatible(
			node, SUNXI_CCU_SUN65IW1_COMPATIBLE))
		return 0xa4U;
	if (sunxi_ccu_dt_is_compatible(
			node, SUNXI_CCU_SUN8IW20_COMPATIBLE) ||
	    sunxi_ccu_dt_is_compatible(
			node, SUNXI_CCU_SUN8IW21_COMPATIBLE))
		return 0xa90U;
	if (sunxi_ccu_dt_is_compatible(
			node, SUNXI_CCU_SUN8IW22_COMPATIBLE))
		return 0x344U;
	return 0U;
}

static inline __attribute__((always_inline)) bool
sunxi_ccu_dt_register(const dt2c_fdt32_t *cells, size_t index,
		      size_t minimum_size, uintptr_t *base, size_t *size) {
	uint32_t raw_base = dt2c_fdt32_to_cpu(cells[index * 2U]);
	uint32_t raw_size = dt2c_fdt32_to_cpu(cells[index * 2U + 1U]);

	if (raw_base == 0U || (raw_base & 3U) != 0U ||
	    raw_size < minimum_size || (raw_size & 3U) != 0U ||
	    raw_base + raw_size < raw_base)
		return false;
	*base = (uintptr_t) raw_base;
	*size = (size_t) raw_size;
	return true;
}

static inline __attribute__((always_inline)) bool
sunxi_ccu_dt_registers(int node, enum sunxi_ccu_dt_layout layout,
		       sunxi_ccu_t *config) {
	const dt2c_fdt32_t *cells;
	size_t count = sunxi_ccu_dt_register_count(layout);
	int length;

	cells = (const dt2c_fdt32_t *) dt2c_fdt_getprop(
			DT2C_FDT_COMPILED_TREE, node, "reg", &length);
	if (cells == NULL || count == 0U ||
	    length != (int) (count * 2U * sizeof(*cells)) ||
	    !sunxi_ccu_dt_register(cells, 0U,
				   sunxi_ccu_dt_main_minimum_size(node),
				   &config->base, &config->size))
		return false;

	if (layout == SUNXI_CCU_DT_LAYOUT_SINGLE)
		return true;
	if (layout == SUNXI_CCU_DT_LAYOUT_APP_AON)
		return sunxi_ccu_dt_register(cells, 1U, 0x58cU,
					     &config->aon_base,
					     &config->aon_size);
	if (layout == SUNXI_CCU_DT_LAYOUT_R_PRCM_IOMMU)
		return sunxi_ccu_dt_register(cells, 1U, 0x314U,
					     &config->r_prcm_base,
					     &config->r_prcm_size) &&
		       sunxi_ccu_dt_register(cells, 2U, 0x44U,
					     &config->iommu_base,
					     &config->iommu_size);
	if (layout == SUNXI_CCU_DT_LAYOUT_R_PRCM_SYSCTRL_IOMMU)
		return sunxi_ccu_dt_register(cells, 1U, 0x258U,
					     &config->r_prcm_base,
					     &config->r_prcm_size) &&
		       sunxi_ccu_dt_register(cells, 2U, 0x164U,
					     &config->sysctrl_base,
					     &config->sysctrl_size) &&
		       sunxi_ccu_dt_register(cells, 3U, 0x44U,
					     &config->iommu_base,
					     &config->iommu_size);
	if (layout == SUNXI_CCU_DT_LAYOUT_CPU_SYS_CFG_R_PRCM_IOMMU)
		return sunxi_ccu_dt_register(cells, 1U, 0x70U,
					     &config->cpu_sys_cfg_base,
					     &config->cpu_sys_cfg_size) &&
		       sunxi_ccu_dt_register(cells, 2U, 0x314U,
					     &config->r_prcm_base,
					     &config->r_prcm_size) &&
		       sunxi_ccu_dt_register(cells, 3U, 0x44U,
					     &config->iommu_base,
					     &config->iommu_size);
	if (layout == SUNXI_CCU_DT_LAYOUT_CPU_PLL) {
		size_t minimum = sunxi_ccu_dt_is_compatible(
				node, SUNXI_CCU_SUN55IW6_COMPATIBLE) ?
				0x50U : 0x24U;

		return sunxi_ccu_dt_register(cells, 1U, minimum,
					     &config->cpu_pll_base,
					     &config->cpu_pll_size);
	}
	if (layout == SUNXI_CCU_DT_LAYOUT_CPU_PLL_RTC)
		return sunxi_ccu_dt_register(cells, 1U, 0x3020U,
					     &config->cpu_pll_base,
					     &config->cpu_pll_size) &&
		       sunxi_ccu_dt_register(cells, 2U, 0x164U,
					     &config->rtc_base,
					     &config->rtc_size);
	return false;
}

static inline __attribute__((always_inline)) int
sunxi_ccu_dt_read_config(sunxi_ccu_t *ccu, int node) {
	enum sunxi_ccu_dt_layout layout;
	sunxi_ccu_t config = {0};

	if (ccu == NULL || node < 0 || !syterkit_dt_node_available(node))
		return DRIVER_ERROR_INVALID;
	layout = sunxi_ccu_dt_layout(node);
	if (layout == SUNXI_CCU_DT_LAYOUT_INVALID ||
	    !sunxi_ccu_dt_registers(node, layout, &config))
		return DRIVER_ERROR_INVALID;

	config.dt_node = node;
	*ccu = config;
	return DRIVER_OK;
}

static inline __attribute__((always_inline)) int
sunxi_ccu_dt_read_alias(sunxi_ccu_t *ccu, const char *alias) {
	if (alias == NULL)
		return DRIVER_ERROR_INVALID;
	return sunxi_ccu_dt_read_config(
			ccu, syterkit_dt_alias_node(alias, NULL));
}

static inline __attribute__((always_inline)) int
sunxi_ccu_dt_read(sunxi_ccu_t *ccu) {
	return sunxi_ccu_dt_read_alias(ccu, "ccu0");
}

#endif /* __DT_COMPATIBLE_CCU_DT_H__ */
