/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file generic.c
 * @brief Common Allwinner Sun60 UFS host glue.
 *
 * The UFSHCI transport does not know about CCU, RTC or the Synopsys TC
 * interface.  This file owns that sequencing and follows the vendor boot
 * flow: clocks and resets, controller-side M-PHY setup, RMMI/C10 tuning and
 * finally the UniPro connection setup around DME LINK STARTUP.
 */

/*
 * Common Allwinner Sun60 UFS host glue.
 *
 * The UFSHCI transport does not know about CCU, RTC or the Synopsys TC
 * interface.  This file owns that sequencing and follows the vendor boot
 * flow: clocks and resets, controller-side M-PHY setup, RMMI/C10 tuning and
 * finally the UniPro connection setup around DME LINK STARTUP.
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <io.h>
#include <log.h>
#include <timer.h>

#include <drivers/sid/sid.h>
#include <drivers/ufs/host/sunxi.h>

#define SUNXI_UFS_MPHY_SRAM_INIT_DONE	(1U << 24)
#define SUNXI_UFS_MPHY_SRAM_BYPASS	(1U << 20)
#define SUNXI_UFS_MPHY_SRAM_EXT_DONE	(1U << 19)

/* Synopsys Test Chip (C10/RMMI) attributes used by the Sun60 PHY. */
#define TC_CBRATSEL		      0x8114U
#define TC_CBC_ADDR_LSB		      0x8116U
#define TC_CBC_ADDR_MSB		      0x8117U
#define TC_CBC_WR_LSB		      0x8118U
#define TC_CBC_WR_MSB		      0x8119U
#define TC_CBC_RD_LSB		      0x811aU
#define TC_CBC_RD_MSB		      0x811bU
#define TC_CBC_RD_WR_SEL	      0x811cU
#define TC_CBC_READ		      0U
#define TC_CBC_WRITE		      1U
#define TC_CBREFCLKCTRL2	      0x8132U
#define TC_CBCRCTRL		      0x811fU
#define TC_EXT_COARSE_TUNE_RATEA      0x814dU
#define TC_EXT_COARSE_TUNE_RATEB      0x814eU
#define TC_RXSQCONTROL		      0x8009U
#define TC_RXRHOLDCTRLOPT	      0x8013U
#define TC_VS_MPHYCFGUPDT	      0xd085U
#define TC_VS_POWERSTATE	      0xd083U
#define TC_VS_MPHY_DISABLE	      0xd0c1U
#define TC_VS_DEBUG_SAVE_CONFIG_TIME  0xd0a0U
#define TC_VS_CLK_MUX_SWITCHING_TIMER 0xd0fbU
#define TC_T_CONNECTIONSTATE	      0x4020U
#define TC_T_CPORTFLAGS		      0x4025U

#define TC_MPLL_PWR_CTL_CAL_CTRL    0x0020U
#define TC_MPLL_SKIPCAL_COARSE_TUNE 0x0028U
#define TC_MPLL_COARSE_TUNE	    0x7014U
#define TC_FAST_FLAGS0		    0x401cU
#define TC_FAST_FLAGS1		    0x411cU
#define TC_AFE_ATT0		    0x4000U
#define TC_AFE_ATT1		    0x4100U
#define TC_AFE_CTLE0		    0x4001U
#define TC_AFE_CTLE1		    0x4101U
#define TC_RX_DFE_TAP3_0	    0x401eU
#define TC_RX_DFE_TAP3_1	    0x411eU
#define TC_RAM_CMN0_B0_R0	    0xc000U

/* Early boot has one active UFS host; each DT parse replaces this variant. */
/** @brief Active Sunxi UFS variant selected by the configured host. */
static struct sunxi_ufs_variant sunxi_active_variant;
/** @brief Whether the active variant has been configured yet. */
static bool sunxi_active_variant_valid;
/** @brief Cached reference-clock type detected from the RTC. */
static uint32_t sunxi_cached_ref_clk_type;
/** @brief Whether the cached reference-clock type is valid. */
static bool sunxi_cached_ref_clk_valid;

/**
 * @brief Return the board-selected Sunxi UFS variant configuration.
 * @return A constant variant descriptor, or a weak default when no
 *         SoC-specific implementation overrides it.
 */
const struct sunxi_ufs_variant *__attribute__((weak)) sunxi_ufs_get_variant(void)
{
	return NULL;
}

/**
 * @brief Read and decode the UFS PHY calibration words from eFuse SRAM.
 * @param[out] cal Receives the decoded PLL and lane calibration values.
 * @return Zero on success, otherwise a UFS host-controller error code.
 */
int __attribute__((weak)) sunxi_get_cal_words(struct sunxi_ufs_cal_words *cal __attribute__((unused)))
{
	return UFSHC_ERR_INVALID;
}

/**
 * @brief Decode the two raw eFuse calibration words.
 * @param[out] cal Receives the decoded calibration fields.
 * @param[in] low Low calibration word read from eFuse SRAM.
 * @param[in] high High calibration word read from eFuse SRAM.
 * @return Zero on success, otherwise a UFS host-controller error code.
 */
int sunxi_decode_cal_words(struct sunxi_ufs_cal_words *cal,
		uint32_t low, uint32_t high)
{
	if (!cal)
		return UFSHC_ERR_INVALID;

	cal->pll_rate_a = (uint16_t)((high >> 16) & 0xffU);
	cal->pll_rate_b = (uint16_t)((high >> 24) & 0xffU);
	cal->att_lane0 = (uint16_t)((low >> 16) & 0xffU);
	cal->ctle_lane0 = (uint16_t)((low >> 24) & 0xffU);
	cal->att_lane1 = (uint16_t)(high & 0xffU);
	cal->ctle_lane1 = (uint16_t)((high >> 8) & 0xffU);

	/* Match the vendor driver's accepted PLL calibration ranges. */
	if (cal->pll_rate_a && cal->pll_rate_b) {
		if (cal->pll_rate_a < 0x14U || cal->pll_rate_a > 0x4bU)
			cal->pll_rate_a = 0;
		if (cal->pll_rate_b < 0x4bU || cal->pll_rate_b > 0x73U)
			cal->pll_rate_b = 0;
	}

	/* A lane pair is usable only when both values are programmed and valid. */
	if (cal->att_lane0 && cal->ctle_lane0 &&
		(cal->att_lane0 == 0U || cal->att_lane0 == 0xffU ||
		 cal->ctle_lane0 == 0U || cal->ctle_lane0 == 0xffU))
		return UFSHC_ERR_IO;
	if (cal->att_lane1 && cal->ctle_lane1 &&
		(cal->att_lane1 == 0U || cal->att_lane1 == 0xffU ||
		 cal->ctle_lane1 == 0U || cal->ctle_lane1 == 0xffU))
		return UFSHC_ERR_IO;
	return 0;
}

/**
 * @brief Merge the selected SoC variant defaults with device-tree settings.
 * @param[out] variant Storage for the active variant configuration.
 * @return Zero on success, otherwise a UFS host-controller error code.
 */
int sunxi_ufs_variant_init(struct sunxi_ufs_variant *variant)
{
	const struct sunxi_ufs_variant *base;

	if (!variant)
		return UFSHC_ERR_INVALID;
	base = sunxi_ufs_get_variant();
	if (!base)
		return UFSHC_ERR_INVALID;
	*variant = *base;
	return 0;
}

/**
 * @brief Return the currently active variant configuration.
 *
 * @return Pointer to the active variant, or the weak default when no active
 *         variant has been configured yet.
 */
static const struct sunxi_ufs_variant *sunxi_variant(void)
{
	if (sunxi_active_variant_valid)
		return &sunxi_active_variant;
	return sunxi_ufs_get_variant();
}

/**
 * @brief Check whether a variant contains all required register fields.
 *
 * @param[in] variant Variant configuration to validate.
 * @return true when the variant is complete and usable.
 */
static bool sunxi_variant_valid(const struct sunxi_ufs_variant *variant)
{
	return variant && variant->reset_reg && variant->axi_clk_reg &&
		variant->cfg_clk_reg && variant->rtc_xo_ctrl &&
		variant->rtc_xo_ctrl1 && variant->rtc_wp && variant->core_rst &&
		variant->phy_rst && variant->axi_rst && variant->bus_rst &&
		variant->ahb_gate && variant->axi_clk_gate &&
		variant->axi_clk_src_mask && variant->axi_clk_factor_mask &&
		variant->cfg_clk_gate && variant->cfg_clk_src_mask &&
		variant->cfg_clk_factor_mask && variant->ufs_cfg_reg &&
		variant->ufs_clk_gate_reg && variant->ufs_cfg_clk_freq_mask &&
		variant->ufs_ref_clk_freq_mask && variant->phy_ref_clk_ctrl &&
		variant->ufs_mphy_cfgclk_gate && variant->ufs_clk24m_gate &&
		variant->rtc_dcxo_gate && variant->rtc_clk_req_disable &&
		variant->rtc_wp_key &&
		variant->rtc_ref_type_mask &&
		variant->rtc_ref_type_shift < 32U;
}

/**
 * @brief Configure Sunxi UFS clocks, resets, RTC, and PHY controls.
 * @param[in] variant Fully initialized variant configuration.
 * @return Zero on success, otherwise a UFS host-controller error code.
 */
int sunxi_ufs_configure(const struct sunxi_ufs_variant *variant)
{
	if (!sunxi_variant_valid(variant))
		return UFSHC_ERR_INVALID;
	sunxi_active_variant = *variant;
	sunxi_active_variant_valid = true;
	sunxi_cached_ref_clk_valid = false;
	return 0;
}

/**
 * @brief Sample the RTC reference-clock type until it is stable.
 *
 * @return The detected reference-clock type, or 0 when no variant is valid.
 */
static uint32_t sunxi_detect_ref_clk_type(void)
{
	const struct sunxi_ufs_variant *variant = sunxi_variant();
	uint32_t previous;
	uint32_t current;
	unsigned int stable = 0;

	if (!sunxi_variant_valid(variant))
		return 0;
	previous = (readl(variant->rtc_xo_ctrl) >> variant->rtc_ref_type_shift) &
		variant->rtc_ref_type_mask;
	current = previous;

	/* The RTC value can change while the oscillator detector settles. */
	while (stable < 3U) {
		current = (readl(variant->rtc_xo_ctrl) >> variant->rtc_ref_type_shift) &
			variant->rtc_ref_type_mask;
		if (current == previous)
			++stable;
		else {
			previous = current;
			stable = 0;
		}
		udelay(3);
	}

	ufs_debug("UFS PHY: reference clock type=%u rtc_xo_ctrl=0x%08x\n", current,
		readl(variant->rtc_xo_ctrl));
	return current;
}

/**
 * @brief Return the cached reference-clock type, detecting it on first use.
 *
 * @return The detected reference-clock type.
 */
static uint32_t sunxi_ref_clk_type(void)
{
	if (!sunxi_cached_ref_clk_valid) {
		sunxi_cached_ref_clk_type = sunxi_detect_ref_clk_type();
		sunxi_cached_ref_clk_valid = true;
	}
	return sunxi_cached_ref_clk_type;
}

/**
 * @brief Report whether a reference-clock type uses an external oscillator.
 *
 * @param[in] variant Variant configuration with external reference types.
 * @param[in] type Reference-clock type to check.
 * @return true when the type is an external reference clock.
 */
static bool sunxi_ref_clk_is_external(const struct sunxi_ufs_variant *variant,
	uint32_t type)
{
	return variant && type < 32U &&
		(variant->rtc_external_ref_types & BIT(type)) != 0U;
}

/**
 * @brief Return the encoded frequency of a reference-clock type.
 *
 * @param[in] variant Variant configuration with the frequency table.
 * @param[in] type Reference-clock type to look up.
 * @return The encoded frequency, or 0 for an unknown type.
 */
static uint32_t sunxi_ref_clk_freq(const struct sunxi_ufs_variant *variant,
	uint32_t type)
{
	if (!variant || type >= 4U)
		return 0;
	return variant->ufs_ref_clk_freq[type];
}

/**
 * @brief Read the device-tree-selected UniPro reference-clock frequency.
 * @param[out] value Receives the encoded reference-clock frequency.
 * @return Zero on success, otherwise a UFS host-controller error code.
 */
int __attribute__((weak)) sunxi_ufs_get_ref_clk_freq(uint32_t *value)
{
	if (!value)
		return UFSHC_ERR_INVALID;
	*value = sunxi_ref_clk_freq(sunxi_variant(), sunxi_ref_clk_type());
	return 0;
}

/**
 * @brief Read the PHY calibration-selected high-speed rate.
 * @param[out] value Receives the supported high-speed rate identifier.
 * @return Zero on success, otherwise a UFS host-controller error code.
 */
int __attribute__((weak)) sunxi_ufs_get_hs_rate(uint32_t *value)
{
	struct sunxi_ufs_cal_words cal;
	int ret;

	if (!value)
		return UFSHC_ERR_INVALID;
	ret = sunxi_get_cal_words(&cal);
	if (ret)
		return ret;
	/* The native platform selects Rate A only when the B calibration word is
	 * absent; otherwise the PHY and PA_HSSERIES both use Rate B. */
	*value = (!cal.pll_rate_b && cal.pll_rate_a) ? UFSHC_HS_RATE_A : UFSHC_HS_RATE_B;
	return 0;
}

/**
 * @brief Initialize the external-resistor calibration for the PHY.
 */
static void sunxi_ext_res_init(void)
{
	const struct sunxi_ufs_variant *variant = sunxi_variant();

	if (!variant || !variant->has_ext_res_cal)
		return;
	/* IW2P1 requires EXT Res200 for the UFS PHY reference resistor. */
	setbits_le32(variant->ext_res_ctrl, BIT(13));
	clrbits_le32(variant->ext_res1_ctrl, 0xffU << 24);
	ufs_debug("UFS PHY: external resistor calibration ext_res=0x%08x ext_res1=0x%08x\n",
		readl(variant->ext_res_ctrl), readl(variant->ext_res1_ctrl));
}

/**
 * @brief Program the UFS configuration clock source, factor, and gate.
 */
static void sunxi_cfg_clk(void)
{
	const struct sunxi_ufs_variant *variant = sunxi_variant();
	uint32_t value;

	if (!sunxi_variant_valid(variant))
		return;
	value = readl(variant->cfg_clk_reg);

	value &= ~variant->cfg_clk_gate;
	writel(value, variant->cfg_clk_reg);
	value &= ~variant->cfg_clk_factor_mask;
	value |= variant->cfg_clk_factor & variant->cfg_clk_factor_mask;
	writel(value, variant->cfg_clk_reg);
	udelay(10);
	value &= ~variant->cfg_clk_src_mask;
	value |= variant->cfg_clk_src_480;
	writel(value, variant->cfg_clk_reg);
	udelay(10);
	/* The native init and deinit paths both leave the configuration clock
	 * gate open after restoring the DT-selected default. */
	value |= variant->cfg_clk_gate;
	writel(value, variant->cfg_clk_reg);
	udelay(10);
}

/**
 * @brief Enable or disable the UFS AXI clock and its reset.
 *
 * @param[in] enable true to enable the AXI clock, false to disable it.
 */
static void sunxi_axi_clk(bool enable)
{
	const struct sunxi_ufs_variant *variant = sunxi_variant();
	uint32_t value;

	if (!sunxi_variant_valid(variant))
		return;
	value = readl(variant->axi_clk_reg);
	value &= ~variant->axi_clk_gate;
	writel(value, variant->axi_clk_reg);
	if (!enable) {
		clrbits_le32(variant->reset_reg, variant->axi_rst);
		clrbits_le32(variant->axi_clk_reg, variant->axi_clk_factor_mask);
		udelay(10);
		clrsetbits_le32(variant->axi_clk_reg, variant->axi_clk_src_mask,
			variant->axi_clk_src_300);
		udelay(10);
		return;
	}
	clrsetbits_le32(variant->axi_clk_reg, variant->axi_clk_src_mask,
		variant->axi_clk_src_200);
	udelay(10);
	clrbits_le32(variant->axi_clk_reg, variant->axi_clk_factor_mask);
	udelay(10);
	setbits_le32(variant->reset_reg, variant->axi_rst);
	setbits_le32(variant->axi_clk_reg, variant->axi_clk_gate);
}

/**
 * @brief Enable or disable the UFS AHB clock and its reset.
 *
 * @param[in] enable true to enable the AHB clock, false to disable it.
 */
static void sunxi_ahb_clk(bool enable)
{
	const struct sunxi_ufs_variant *variant = sunxi_variant();

	if (!sunxi_variant_valid(variant))
		return;
	if (enable) {
		setbits_le32(variant->reset_reg, variant->bus_rst);
		setbits_le32(variant->reset_reg, variant->ahb_gate);
	} else {
		clrbits_le32(variant->reset_reg, variant->ahb_gate);
		clrbits_le32(variant->reset_reg, variant->bus_rst);
	}
}

/**
 * @brief Enable or disable the RTC reference clock.
 *
 * @param[in] enable true to enable the reference clock, false to disable it.
 */
static void sunxi_ref_clk(bool enable)
{
	const struct sunxi_ufs_variant *variant = sunxi_variant();
	uint32_t value;
	bool external = false;

	if (!variant)
		return;

	if (enable) {
		uint32_t type = sunxi_ref_clk_type();

		external = sunxi_ref_clk_is_external(variant, type);
	}

	/* RTC write protection uses the low half-word key. */
	value = readl(variant->rtc_wp) & ~0xffffU;
	writel(value | variant->rtc_wp_key, variant->rtc_wp);
	if (enable) {
		if (external) {
			setbits_le32(variant->rtc_xo_ctrl, variant->rtc_clk_req_disable);
			clrbits_le32(variant->rtc_xo_ctrl1, variant->rtc_dcxo_gate);
		} else {
			setbits_le32(variant->rtc_xo_ctrl1, variant->rtc_dcxo_gate);
			clrbits_le32(variant->rtc_xo_ctrl, variant->rtc_clk_req_disable);
		}
	} else {
		setbits_le32(variant->rtc_xo_ctrl, variant->rtc_clk_req_disable);
		clrbits_le32(variant->rtc_xo_ctrl1, variant->rtc_dcxo_gate);
	}
}

/**
 * @brief Enable the Sunxi UFS power and clock resources.
 * @return Zero on success, otherwise a UFS host-controller error code.
 */
int __attribute__((weak)) sunxi_ufs_enable(void)
{
	const struct sunxi_ufs_variant *variant = sunxi_variant();

	if (!sunxi_variant_valid(variant))
		return UFSHC_ERR_INVALID;
	/* Native startup samples the oscillator detector once and reuses it for
	 * both top-level clock routing and the UFS_CFG frequency fields. */
	sunxi_cached_ref_clk_valid = false;
	sunxi_ref_clk_type();
	sunxi_ext_res_init();
	/* Assert the bus, AXI, core and PHY resets before changing clocks. */
	sunxi_ahb_clk(false);
	sunxi_axi_clk(false);
	sunxi_axi_clk(true);
	sunxi_ahb_clk(true);
	clrbits_le32(variant->reset_reg, variant->core_rst | variant->phy_rst);
	sunxi_cfg_clk();
	sunxi_ref_clk(true);
	return 0;
}

/**
 * @brief Prepare the Sunxi UFS PHY and controller for link startup.
 * @return Zero on success, otherwise a UFS host-controller error code.
 */
int __attribute__((weak)) sunxi_ufs_prepare(void)
{
	const struct sunxi_ufs_variant *variant = sunxi_variant();
	uint32_t value;
	uint32_t ref_type;
	uint32_t ref_freq;
	bool external;

	if (!sunxi_variant_valid(variant))
		return UFSHC_ERR_INVALID;
	ref_type = sunxi_ref_clk_type();
	ref_freq = sunxi_ref_clk_freq(variant, ref_type);
	external = sunxi_ref_clk_is_external(variant, ref_type);

	value = readl(variant->ufs_cfg_reg);
	value &= ~(SUNXI_UFS_MPHY_SRAM_BYPASS | SUNXI_UFS_MPHY_SRAM_EXT_DONE |
		   variant->ufs_cfg_clk_freq_mask | variant->ufs_ref_clk_freq_mask |
		   variant->ufs_ref_clk_unipro_sel | variant->ufs_ref_clk_app_sel);
	value |= (variant->ufs_cfg_clk_freq & variant->ufs_cfg_clk_freq_mask) |
		(ref_freq & variant->ufs_ref_clk_freq_mask) | variant->ufs_ref_clk_app_enable;
	if (external)
		value |= variant->ufs_ref_clk_unipro_sel | variant->ufs_ref_clk_app_sel;
	writel(value, variant->ufs_cfg_reg);

	/* Keep the M-PHY configuration and 24 MHz clocks ungated. */
	value = readl(variant->ufs_clk_gate_reg);
	value &= ~(variant->ufs_mphy_cfgclk_gate | variant->ufs_clk24m_gate);
	/* Disable the controller power/clock auto-gates after power-on, as in
	 * the native Sun60 host-init sequence. */
	value |= variant->ufs_clk_gate_autogate_off;
	writel(value, variant->ufs_clk_gate_reg);

	/* Release core reset only after UFS_CFG and clock-gate inputs settle. */
	setbits_le32(variant->reset_reg, variant->core_rst);

	return 0;
}

/**
 * @brief Reset the Sunxi UFS device-side PHY/controller path.
 */
void __attribute__((weak)) sunxi_ufs_device_reset(void)
{
	const struct sunxi_ufs_variant *variant = sunxi_variant();
	uint32_t value;

	if (!sunxi_variant_valid(variant))
		return;
	value = readl(variant->ufs_cfg_reg);

	/* UFS_CFG[2] is the active-low device RST_n output.  Match the native
	 * platform pulse and its post-release settling delay. */
	value &= ~BIT(2);
	writel(value, variant->ufs_cfg_reg);
	udelay(15);
	value |= BIT(2);
	writel(value, variant->ufs_cfg_reg);
	mdelay(15);
}

/**
 * @brief Write a DME attribute through the host controller.
 *
 * @param[in,out] host Initialized UFS host-controller state.
 * @param[in] attr DME attribute identifier.
 * @param[in] selector Attribute selector.
 * @param[in] value Value to write.
 * @return Zero on success, otherwise a UFS host-controller error code.
 */
static int sunxi_dme_write(struct ufshc_host *host, uint32_t attr, uint16_t selector, uint32_t value)
{
	return ufshc_dme_set_sel(host, attr, selector, value, false);
}

/**
 * @brief Read a DME attribute through the host controller.
 *
 * @param[in,out] host Initialized UFS host-controller state.
 * @param[in] attr DME attribute identifier.
 * @param[in] selector Attribute selector.
 * @param[out] value Receives the read value.
 * @return Zero on success, otherwise a UFS host-controller error code.
 */
static int sunxi_dme_read(struct ufshc_host *host, uint32_t attr, uint16_t selector, uint32_t *value)
{
	return ufshc_dme_get_sel(host, attr, selector, value, false);
}

/**
 * @brief Read a 16-bit Synopsys Test Chip register.
 *
 * @param[in,out] host Initialized UFS host-controller state.
 * @param[in] reg Test Chip register address.
 * @param[out] value Receives the read value.
 * @return Zero on success, otherwise a UFS host-controller error code.
 */
static int sunxi_c10_read(struct ufshc_host *host, uint16_t reg, uint16_t *value)
{
	uint32_t low;
	uint32_t high;
	int ret;

	ret = sunxi_dme_write(host, TC_CBC_ADDR_LSB, 0, reg & 0xffU);
	if (ret)
		return ret;
	ret = sunxi_dme_write(host, TC_CBC_ADDR_MSB, 0, reg >> 8);
	if (ret)
		return ret;
	ret = sunxi_dme_write(host, TC_CBC_RD_WR_SEL, 0, TC_CBC_READ);
	if (ret)
		return ret;
	ret = sunxi_dme_write(host, TC_VS_MPHYCFGUPDT, 0, 1);
	if (ret)
		return ret;
	ret = sunxi_dme_read(host, TC_CBC_RD_LSB, 0, &low);
	if (ret)
		return ret;
	ret = sunxi_dme_read(host, TC_CBC_RD_MSB, 0, &high);
	if (ret)
		return ret;
	*value = (uint16_t)((high << 8) | low);
	return 0;
}

/**
 * @brief Write a 16-bit Synopsys Test Chip register.
 *
 * @param[in,out] host Initialized UFS host-controller state.
 * @param[in] reg Test Chip register address.
 * @param[in] value Value to write.
 * @return Zero on success, otherwise a UFS host-controller error code.
 */
static int sunxi_c10_write(struct ufshc_host *host, uint16_t reg, uint16_t value)
{
	int ret;

	ret = sunxi_dme_write(host, TC_CBC_ADDR_LSB, 0, reg & 0xffU);
	if (ret)
		return ret;
	ret = sunxi_dme_write(host, TC_CBC_ADDR_MSB, 0, reg >> 8);
	if (ret)
		return ret;
	ret = sunxi_dme_write(host, TC_CBC_WR_LSB, 0, value & 0xffU);
	if (ret)
		return ret;
	ret = sunxi_dme_write(host, TC_CBC_WR_MSB, 0, value >> 8);
	if (ret)
		return ret;
	ret = sunxi_dme_write(host, TC_CBC_RD_WR_SEL, 0, TC_CBC_WRITE);
	if (ret)
		return ret;
	return sunxi_dme_write(host, TC_VS_MPHYCFGUPDT, 0, 1);
}

/**
 * @brief Perform the Sunxi-specific UniPro link-startup sequence.
 * @param[in,out] host Initialized UFS host-controller state.
 * @return Zero when the link starts, otherwise a UFS host-controller error code.
 */
int __attribute__((weak)) sunxi_ufs_link_startup(struct ufshc_host *host)
{
	const struct sunxi_ufs_variant *variant = sunxi_variant();
	struct sunxi_ufs_cal_words cal = { 0 };
	struct {
		uint32_t attr;
		uint16_t selector;
		uint32_t value;
	} rmmi[] = {
		{ TC_CBRATSEL, 0, 1 }, /* changed to Rate A when only A is fused */
		{ TC_CBREFCLKCTRL2, 0, 0 },
		{ TC_RXSQCONTROL, 4, 1 },
		{ TC_RXSQCONTROL, 5, 1 },
		{ TC_RXRHOLDCTRLOPT, 4, 2 },
		{ TC_RXRHOLDCTRLOPT, 5, 2 },
		{ TC_EXT_COARSE_TUNE_RATEA, 0, 2 },
		{ TC_EXT_COARSE_TUNE_RATEB, 0, 0x80 },
		{ TC_CBCRCTRL, 0, 1 },
		{ TC_VS_MPHYCFGUPDT, 0, 1 },
	};
	struct {
		uint16_t reg;
		uint16_t value;
	} c10[] = {
		{ TC_FAST_FLAGS0, 4 },
		{ TC_FAST_FLAGS1, 4 },
	};
	struct {
		uint16_t reg;
		uint16_t value;
	} afe_cal[] = {
		{ TC_AFE_ATT0, 0x80 },
		{ TC_AFE_ATT1, 0x80 },
		{ TC_AFE_CTLE0, 0x80 },
		{ TC_AFE_CTLE1, 0x80 },
		{ TC_RX_DFE_TAP3_0, 0xa00 },
		{ TC_RX_DFE_TAP3_1, 0xa00 },
	};
	uint64_t deadline;
	uint32_t value;
	uint16_t scratch;
	int ret;

	if (!sunxi_variant_valid(variant))
		return UFSHC_ERR_INVALID;
	rmmi[1].value = variant->phy_ref_clk_ctrl;
	ret = sunxi_get_cal_words(&cal);
	if (ret)
		return ret;
	if (!cal.pll_rate_b && cal.pll_rate_a)
		rmmi[0].value = 0; /* Rate A */
	if (cal.pll_rate_a)
		rmmi[6].value = cal.pll_rate_a;
	if (cal.pll_rate_b)
		rmmi[7].value = cal.pll_rate_b;
	/* A failed LINK STARTUP may leave the PHY out of reset.  Reassert it so
	 * each retry starts from the same RMMI state as the first attempt. */
	clrbits_le32(variant->reset_reg, variant->phy_rst);
	udelay(2);
	for (size_t i = 0; i < sizeof(rmmi) / sizeof(rmmi[0]); ++i) {
		ret = sunxi_dme_write(host, rmmi[i].attr, rmmi[i].selector, rmmi[i].value);
		if (ret)
			return ret;
	}

	/* Release PHY reset only after the RMMI block has been configured. */
	setbits_le32(variant->reset_reg, variant->phy_rst);
	/* The vendor startup sequence specifies a 10 ms SRAM-init window.  Keep
	 * this PHY-local deadline independent of the longer HCI transaction timeout. */
	deadline = time_us() + UFSHC_PHY_INIT_TIMEOUT_US;
	while (!(readl(variant->ufs_cfg_reg) & SUNXI_UFS_MPHY_SRAM_INIT_DONE)) {
		uint64_t now = time_us();

		if (now >= deadline) {
			ufs_debug("UFS PHY: M-PHY SRAM init timeout cfg=0x%08x\n", readl(variant->ufs_cfg_reg));
			return UFSHC_ERR_TIMEOUT;
		}
	}

	/* Exercise the indirect SRAM path as in the vendor sequence. */
	ret = sunxi_c10_read(host, TC_RAM_CMN0_B0_R0 + 2U, &scratch);
	if (ret) {
		ufs_debug("UFS PHY: C10 SRAM read failed ret=%d\n", ret);
		return ret;
	}
	ret = sunxi_c10_write(host, TC_RAM_CMN0_B0_R0 + 2U, scratch);
	if (ret) {
		ufs_debug("UFS PHY: C10 SRAM write failed ret=%d\n", ret);
		return ret;
	}
	if (cal.pll_rate_a || cal.pll_rate_b) {
		uint16_t coarse = cal.pll_rate_b ? cal.pll_rate_b : cal.pll_rate_a;

		ret = sunxi_c10_write(host, TC_MPLL_COARSE_TUNE, coarse);
		if (ret) {
			ufs_debug("UFS PHY: C10 MPLL coarse tune failed ret=%d\n", ret);
			return ret;
		}
		ret = sunxi_c10_write(host, TC_MPLL_SKIPCAL_COARSE_TUNE, coarse);
		if (ret) {
			ufs_debug("UFS PHY: C10 MPLL skip calibration failed ret=%d\n", ret);
			return ret;
		}
	}
	for (size_t i = 0; i < sizeof(c10) / sizeof(c10[0]); ++i) {
		ret = sunxi_c10_write(host, c10[i].reg, c10[i].value);
		if (ret) {
			ufs_debug("UFS PHY: C10 tuning reg=0x%04x failed ret=%d\n", c10[i].reg, ret);
			return ret;
		}
	}
	if (cal.att_lane0 && cal.ctle_lane0 && cal.att_lane1 && cal.ctle_lane1) {
		afe_cal[0].value = cal.att_lane0;
		afe_cal[1].value = cal.att_lane1;
		afe_cal[2].value = cal.ctle_lane0;
		afe_cal[3].value = cal.ctle_lane1;
		for (size_t i = 0; i < sizeof(afe_cal) / sizeof(afe_cal[0]); ++i) {
			ret = sunxi_c10_write(host, afe_cal[i].reg, afe_cal[i].value);
			if (ret) {
				ufs_debug("UFS PHY: C10 AFE calibration reg=0x%04x failed ret=%d\n",
					afe_cal[i].reg, ret);
				return ret;
			}
		}
	}
	if (!cal.pll_rate_a && !cal.pll_rate_b) {
		/* No fused PLL words: trigger the M-PHY automatic calibration. */
		ret = sunxi_c10_write(host, TC_MPLL_PWR_CTL_CAL_CTRL, 8);
		if (ret) {
			ufs_debug("UFS PHY: automatic MPLL calibration failed ret=%d\n", ret);
			return ret;
		}
	}

	value = readl(variant->ufs_cfg_reg);
	value |= SUNXI_UFS_MPHY_SRAM_EXT_DONE;
	writel(value, variant->ufs_cfg_reg);
	ret = sunxi_dme_write(host, TC_VS_MPHYCFGUPDT, 0, 1);
	if (ret) {
		ufs_debug("UFS PHY: M-PHY configuration update failed ret=%d\n", ret);
		return ret;
	}
	ret = sunxi_dme_write(host, TC_VS_MPHY_DISABLE, 0, 0);
	if (ret) {
		ufs_debug("UFS PHY: M-PHY enable failed ret=%d\n", ret);
		return ret;
	}
	ret = sunxi_dme_write(host, TC_VS_DEBUG_SAVE_CONFIG_TIME, 0, 0x1b);
	if (ret) {
		ufs_debug("UFS PHY: debug save-config timer setup failed ret=%d\n", ret);
		return ret;
	}
	ret = sunxi_dme_write(host, TC_VS_CLK_MUX_SWITCHING_TIMER, 0, 0xa);
	if (ret)
		ufs_debug("UFS PHY: clock mux timer setup failed ret=%d\n", ret);
	return ret;
}

/**
 * @brief Complete Sunxi-specific link setup after UFSHCI startup.
 * @param[in,out] host Initialized UFS host-controller state.
 * @return Zero when the link is usable, otherwise a UFS host-controller error code.
 */
int __attribute__((weak)) sunxi_ufs_link_up(struct ufshc_host *host)
{
	uint32_t value = 0;
	int ret;

	ret = sunxi_dme_read(host, TC_VS_POWERSTATE, 0, &value);
	if (ret || value != 2U) {
		ufs_debug("UFS PHY: invalid power state value=%u ret=%d\n", value, ret);
		return ret ? ret : UFSHC_ERR_IO;
	}
	ret = sunxi_dme_write(host, TC_T_CONNECTIONSTATE, 0, 0);
	if (ret) {
		ufs_debug("UFS PHY: clear connection state failed ret=%d\n", ret);
		return ret;
	}
	ret = sunxi_dme_write(host, TC_T_CPORTFLAGS, 0, 6);
	if (ret) {
		ufs_debug("UFS PHY: set CPort flags failed ret=%d\n", ret);
		return ret;
	}
	ret = sunxi_dme_write(host, TC_T_CONNECTIONSTATE, 0, 1);
	if (ret) {
		ufs_debug("UFS PHY: set connection state failed ret=%d\n", ret);
		return ret;
	}
	ret = sunxi_dme_read(host, TC_T_CONNECTIONSTATE, 0, &value);
	if (ret || value != 1U)
		ufs_debug("UFS PHY: connection state verify failed value=%u ret=%d\n", value, ret);
	return ret ? ret : (value == 1U ? 0 : UFSHC_ERR_IO);
}

/**
 * @brief Disable the Sunxi UFS resources after controller shutdown.
 */
void __attribute__((weak)) sunxi_ufs_disable(void)
{
	const struct sunxi_ufs_variant *variant = sunxi_variant();

	if (!sunxi_variant_valid(variant))
		return;
	/* Both core and PHY reset inputs are active low. */
	clrbits_le32(variant->reset_reg, variant->phy_rst);
	clrbits_le32(variant->reset_reg, variant->core_rst);
	sunxi_ref_clk(false);
	sunxi_cfg_clk();
	sunxi_ahb_clk(false);
	sunxi_axi_clk(false);
	sunxi_cached_ref_clk_valid = false;
}
