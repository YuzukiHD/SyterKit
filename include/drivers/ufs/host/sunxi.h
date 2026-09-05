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
/**
 * @brief Sunxi UFS PHY and clock configuration for one SoC variant.
 *
 * The generic Sunxi host implementation consumes this table to configure
 * reset, clock, RTC reference-clock, and PHY calibration registers without
 * selecting a SoC through preprocessor branches.
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

/**
 * @brief Decoded UFS PHY calibration values read from the SoC eFuse SRAM.
 */
struct sunxi_ufs_cal_words {
	uint16_t pll_rate_a;
	uint16_t pll_rate_b;
	uint16_t att_lane0;
	uint16_t ctle_lane0;
	uint16_t att_lane1;
	uint16_t ctle_lane1;
};

/* Weak defaults live in the common implementation and are overridden by a
 * selected Sun60 variant object. */
/**
 * @brief Return the board-selected Sunxi UFS variant configuration.
 * @return A constant variant descriptor, or a weak default when no
 *         SoC-specific implementation overrides it.
 */
const struct sunxi_ufs_variant *sunxi_ufs_get_variant(void);
/**
 * @brief Read and decode the UFS PHY calibration words from eFuse SRAM.
 * @param[out] cal Receives the decoded PLL and lane calibration values.
 * @return Zero on success, otherwise a UFS host-controller error code.
 */
int sunxi_get_cal_words(struct sunxi_ufs_cal_words *cal);

/* Shared validation/decoding for the two eFuse layouts. */
/**
 * @brief Decode the two raw eFuse calibration words.
 * @param[out] cal Receives the decoded calibration fields.
 * @param[in] low Low calibration word read from eFuse SRAM.
 * @param[in] high High calibration word read from eFuse SRAM.
 * @return Zero on success, otherwise @ref UFSHC_ERR_INVALID.
 */
int sunxi_decode_cal_words(struct sunxi_ufs_cal_words *cal,
		uint32_t low, uint32_t high);

/**
 * @brief Merge the selected SoC variant defaults with device-tree settings.
 * @param[out] variant Storage for the active variant configuration.
 * @return Zero on success, otherwise a UFS host-controller error code.
 */
int sunxi_ufs_variant_init(struct sunxi_ufs_variant *variant);
/**
 * @brief Configure Sunxi UFS clocks, resets, RTC, and PHY controls.
 * @param[in] variant Fully initialized variant configuration.
 * @return Zero on success, otherwise a UFS host-controller error code.
 */
int sunxi_ufs_configure(const struct sunxi_ufs_variant *variant);

/**
 * @brief Enable the Sunxi UFS power and clock resources.
 * @return Zero on success, otherwise a UFS host-controller error code.
 */
int sunxi_ufs_enable(void);
/**
 * @brief Disable the Sunxi UFS resources after controller shutdown.
 */
void sunxi_ufs_disable(void);
/**
 * @brief Prepare the Sunxi UFS PHY and controller for link startup.
 * @return Zero on success, otherwise a UFS host-controller error code.
 */
int sunxi_ufs_prepare(void);
/**
 * @brief Reset the Sunxi UFS device-side PHY/controller path.
 */
void sunxi_ufs_device_reset(void);
/**
 * @brief Read the device-tree-selected UniPro reference-clock frequency.
 * @param[out] value Receives the encoded reference-clock frequency.
 * @return Zero on success, otherwise a UFS host-controller error code.
 */
int sunxi_ufs_get_ref_clk_freq(uint32_t *value);
/**
 * @brief Read the PHY calibration-selected high-speed rate.
 * @param[out] value Receives the supported high-speed rate identifier.
 * @return Zero on success, otherwise a UFS host-controller error code.
 */
int sunxi_ufs_get_hs_rate(uint32_t *value);
/**
 * @brief Perform the Sunxi-specific UniPro link-startup sequence.
 * @param[in,out] host Initialized UFS host-controller state.
 * @return Zero when the link starts, otherwise a UFS host-controller error code.
 */
int sunxi_ufs_link_startup(struct ufshc_host *host);
/**
 * @brief Complete Sunxi-specific link setup after UFSHCI startup.
 * @param[in,out] host Initialized UFS host-controller state.
 * @return Zero when the link is usable, otherwise a UFS host-controller error code.
 */
int sunxi_ufs_link_up(struct ufshc_host *host);

#endif /* __SYTERKIT_UFS_SUNXI_HOST_H__ */
