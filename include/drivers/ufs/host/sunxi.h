/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __SYTERKIT_UFS_SUNXI_HOST_H__
#define __SYTERKIT_UFS_SUNXI_HOST_H__

#include <stdbool.h>
#include <stdint.h>

#include <drivers/ufs/ufshc.h>

/*
 * Register and calibration data which differs between Sun60 revisions.
 * The host flow in generic.c consumes this table without selecting a chip
 * through preprocessor branches.
 */
struct sunxi_ufs_variant {
	uintptr_t reset_reg;
	uintptr_t axi_clk_reg;
	uintptr_t cfg_clk_reg;
	uintptr_t rtc_xo_ctrl;
	uintptr_t rtc_xo_ctrl1;
	uintptr_t rtc_wp;
	uintptr_t ufs_cfg_reg;
	uintptr_t ufs_clk_gate_reg;

	uint32_t core_rst;
	uint32_t phy_rst;
	uint32_t axi_rst;
	uint32_t bus_rst;
	uint32_t ahb_gate;

	uint32_t axi_clk_gate;
	uint32_t axi_clk_src_mask;
	uint32_t axi_clk_src_300;
	uint32_t axi_clk_src_200;
	uint32_t axi_clk_factor_mask;

	uint32_t cfg_clk_gate;
	uint32_t cfg_clk_src_mask;
	uint32_t cfg_clk_src_480;
	uint32_t cfg_clk_factor_mask;
	uint32_t cfg_clk_factor;

	/* UFS controller clock routing and gate fields supplied by DT. */
	uint32_t ufs_cfg_clk_freq_mask;
	uint32_t ufs_cfg_clk_freq;
	uint32_t ufs_ref_clk_unipro_sel;
	uint32_t ufs_ref_clk_app_sel;
	uint32_t ufs_ref_clk_app_enable;
	uint32_t ufs_ref_clk_freq_mask;
	uint32_t ufs_ref_clk_freq[4];
	uint32_t ufs_mphy_cfgclk_gate;
	uint32_t ufs_clk24m_gate;
	uint32_t ufs_clk_gate_autogate_off;
	/* RMMI reference-clock control value supplied by DT. */
	uint32_t phy_ref_clk_ctrl;

	/* RTC oscillator detection and gate fields supplied by DT. */
	uint32_t rtc_ref_type_shift;
	uint32_t rtc_ref_type_mask;
	uint32_t rtc_external_ref_types;
	uint32_t rtc_dcxo_gate;
	uint32_t rtc_clk_req_disable;
	uint32_t rtc_wp_key;

	uint32_t cal_low_offset;
	uint32_t cal_high_offset;
	uintptr_t ext_res_ctrl;
	uintptr_t ext_res1_ctrl;
	bool has_ext_res_cal;
};

struct sunxi_ufs_cal_words {
	uint16_t pll_rate_a;
	uint16_t pll_rate_b;
	uint16_t att_lane0;
	uint16_t ctle_lane0;
	uint16_t att_lane1;
	uint16_t ctle_lane1;
};

struct sunxi_ufs_platform_data {
	struct sunxi_ufs_variant variant;
	uint32_t ref_clk_type;
	bool ref_clk_type_valid;
};

/* Weak defaults live in the common implementation and are overridden by a
 * selected Sun60 variant object. */
const struct sunxi_ufs_variant *sunxi_ufs_get_variant(void);
int sunxi_get_cal_words(struct sunxi_ufs_cal_words *cal);

/* Shared validation/decoding for the two eFuse layouts. */
int sunxi_decode_cal_words(struct sunxi_ufs_cal_words *cal,
		uint32_t low, uint32_t high);

int sunxi_ufs_platform_data_init(struct sunxi_ufs_platform_data *data);

extern const struct ufshc_platform_ops sunxi_ufs_platform_ops;

#endif /* __SYTERKIT_UFS_SUNXI_HOST_H__ */
