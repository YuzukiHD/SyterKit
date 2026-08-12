/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __DRIVERS_CLK_H__
#define __DRIVERS_CLK_H__

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sunxi_ccu {
	int dt_node;
	uintptr_t base;
	size_t size;
	uintptr_t aon_base;
	size_t aon_size;
	uintptr_t cpu_pll_base;
	size_t cpu_pll_size;
	uintptr_t cpu_sys_cfg_base;
	size_t cpu_sys_cfg_size;
	uintptr_t r_prcm_base;
	size_t r_prcm_size;
	uintptr_t sysctrl_base;
	size_t sysctrl_size;
	uintptr_t iommu_base;
	size_t iommu_size;
	uintptr_t rtc_base;
	size_t rtc_size;
} sunxi_ccu_t;

typedef struct {
	uintptr_t gate_reg_base;
	uint32_t gate_reg_offset;
	uintptr_t rst_reg_base;
	uint32_t rst_reg_offset;
	uint32_t parent_clk;
} sunxi_clk_t;

static inline __attribute__((always_inline)) uintptr_t
sunxi_ccu_reg(const sunxi_ccu_t *ccu, uintptr_t offset) {
	return ccu->base + offset;
}

static inline __attribute__((always_inline)) uintptr_t
sunxi_ccu_aon_reg(const sunxi_ccu_t *ccu, uintptr_t offset) {
	return ccu->aon_base + offset;
}

static inline __attribute__((always_inline)) uintptr_t
sunxi_ccu_cpu_pll_reg(const sunxi_ccu_t *ccu, uintptr_t offset) {
	return ccu->cpu_pll_base + offset;
}

static inline __attribute__((always_inline)) uintptr_t
sunxi_ccu_cpu_sys_cfg_reg(const sunxi_ccu_t *ccu, uintptr_t offset) {
	return ccu->cpu_sys_cfg_base + offset;
}

static inline __attribute__((always_inline)) uintptr_t
sunxi_ccu_r_prcm_reg(const sunxi_ccu_t *ccu, uintptr_t offset) {
	return ccu->r_prcm_base + offset;
}

static inline __attribute__((always_inline)) uintptr_t
sunxi_ccu_sysctrl_reg(const sunxi_ccu_t *ccu, uintptr_t offset) {
	return ccu->sysctrl_base + offset;
}

static inline __attribute__((always_inline)) uintptr_t
sunxi_ccu_iommu_reg(const sunxi_ccu_t *ccu, uintptr_t offset) {
	return ccu->iommu_base + offset;
}

static inline __attribute__((always_inline)) uintptr_t
sunxi_ccu_rtc_reg(const sunxi_ccu_t *ccu, uintptr_t offset) {
	return ccu->rtc_base + offset;
}

/** @brief Initialize the system clock tree. */
void sunxi_clk_init(sunxi_ccu_t *ccu);

/** @brief Initialize clocks required before normal device initcalls. */
void sunxi_clk_pre_init(sunxi_ccu_t *ccu);

/** @brief Return the high-speed oscillator frequency in MHz. */
uint32_t sunxi_clk_get_hosc_type(sunxi_ccu_t *ccu);

/** @brief Reset the system clock tree to its boot configuration. */
void sunxi_clk_reset(sunxi_ccu_t *ccu);

/** @brief Dump the current system clock configuration. */
void sunxi_clk_dump(sunxi_ccu_t *ccu);

/** @brief Return the PERI1X clock rate. */
uint32_t sunxi_clk_get_peri1x_rate(sunxi_ccu_t *ccu);

/** @brief Initialize the USB clock gates. */
void sunxi_usb_clk_init(sunxi_ccu_t *ccu);

/** @brief Disable the USB clock gates. */
void sunxi_usb_clk_deinit(sunxi_ccu_t *ccu);

/** @brief Set the CPU PLL frequency. */
void sunxi_clk_set_cpu_pll(sunxi_ccu_t *ccu, uint32_t freq);

#ifdef __cplusplus
}
#endif

#endif /* __DRIVERS_CLK_H__ */
