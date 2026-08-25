/* SPDX-License-Identifier: GPL-2.0+ */

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

#include <drivers/soc/sid.h>
#include <drivers/ufs/host.h>
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

static uint32_t sunxi_cached_ref_clk_type;
static bool sunxi_cached_ref_clk_valid;

static uint32_t *sunxi_ref_clk_cache_type(void *priv)
{
	if (priv)
		return &((struct sunxi_ufs_platform_data *)priv)->ref_clk_type;
	return &sunxi_cached_ref_clk_type;
}

static bool *sunxi_ref_clk_cache_valid(void *priv)
{
	if (priv)
		return &((struct sunxi_ufs_platform_data *)priv)->ref_clk_type_valid;
	return &sunxi_cached_ref_clk_valid;
}

const struct sunxi_ufs_variant *__attribute__((weak)) sunxi_ufs_get_variant(void)
{
	return NULL;
}

int __attribute__((weak)) sunxi_get_cal_words(struct sunxi_ufs_cal_words *cal)
{
	(void)cal;
	return UFSHC_ERR_INVALID;
}

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

int sunxi_ufs_platform_data_init(struct sunxi_ufs_platform_data *data)
{
	const struct sunxi_ufs_variant *variant;

	if (!data)
		return UFSHC_ERR_INVALID;
	variant = sunxi_ufs_get_variant();
	if (!variant)
		return UFSHC_ERR_INVALID;
	data->variant = *variant;
	data->ref_clk_type = 0;
	data->ref_clk_type_valid = false;
	return 0;
}

static const struct sunxi_ufs_variant *sunxi_variant(void *priv)
{
	if (priv)
		return &((struct sunxi_ufs_platform_data *)priv)->variant;
	return sunxi_ufs_get_variant();
}

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

static uint32_t sunxi_detect_ref_clk_type(void *priv)
{
	const struct sunxi_ufs_variant *variant = sunxi_variant(priv);
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

	return current;
}

static uint32_t sunxi_ref_clk_type(void *priv)
{
	uint32_t *type = sunxi_ref_clk_cache_type(priv);
	bool *valid = sunxi_ref_clk_cache_valid(priv);

	if (!*valid) {
		*type = sunxi_detect_ref_clk_type(priv);
		*valid = true;
	}
	return *type;
}

static bool sunxi_ref_clk_is_external(const struct sunxi_ufs_variant *variant,
	uint32_t type)
{
	return variant && type < 32U &&
		(variant->rtc_external_ref_types & BIT(type)) != 0U;
}

static uint32_t sunxi_ref_clk_freq(const struct sunxi_ufs_variant *variant,
	uint32_t type)
{
	if (!variant || type >= 4U)
		return 0;
	return variant->ufs_ref_clk_freq[type];
}

static int sunxi_get_ref_clk_freq(void *priv, uint32_t *value)
{
	if (!value)
		return UFSHC_ERR_INVALID;
	*value = sunxi_ref_clk_freq(sunxi_variant(priv), sunxi_ref_clk_type(priv));
	return 0;
}

static int sunxi_get_hs_rate(void *priv __attribute__((unused)), uint32_t *value)
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

static void sunxi_ext_res_init(void *priv)
{
	const struct sunxi_ufs_variant *variant = sunxi_variant(priv);

	if (!variant || !variant->has_ext_res_cal)
		return;
	/* IW2P1 requires EXT Res200 for the UFS PHY reference resistor. */
	setbits_le32(variant->ext_res_ctrl, BIT(13));
	clrbits_le32(variant->ext_res1_ctrl, 0xffU << 24);
}

static void sunxi_cfg_clk(void *priv)
{
	const struct sunxi_ufs_variant *variant = sunxi_variant(priv);
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

static void sunxi_axi_clk(bool enable, void *priv)
{
	const struct sunxi_ufs_variant *variant = sunxi_variant(priv);
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

static void sunxi_ahb_clk(bool enable, void *priv)
{
	const struct sunxi_ufs_variant *variant = sunxi_variant(priv);

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

static void sunxi_ref_clk(bool enable, void *priv)
{
	const struct sunxi_ufs_variant *variant = sunxi_variant(priv);
	uint32_t value;
	bool external = false;

	if (!variant)
		return;

	if (enable) {
		uint32_t type = sunxi_ref_clk_type(priv);

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

static int sunxi_enable(void *priv)
{
	if (!sunxi_variant_valid(sunxi_variant(priv)))
		return UFSHC_ERR_INVALID;
	/* Native startup samples the oscillator detector once and reuses it for
	 * both top-level clock routing and the UFS_CFG frequency fields. */
	*sunxi_ref_clk_cache_valid(priv) = false;
	(void)sunxi_ref_clk_type(priv);
	sunxi_ext_res_init(priv);
	/* Assert the bus, AXI, core and PHY resets before changing clocks. */
	sunxi_ahb_clk(false, priv);
	sunxi_axi_clk(false, priv);
	sunxi_axi_clk(true, priv);
	sunxi_ahb_clk(true, priv);
	{
		const struct sunxi_ufs_variant *variant = sunxi_variant(priv);

		clrbits_le32(variant->reset_reg, variant->core_rst | variant->phy_rst);
	}
	sunxi_cfg_clk(priv);
	sunxi_ref_clk(true, priv);
	return 0;
}

static int sunxi_prepare(uintptr_t base, void *priv)
{
	const struct sunxi_ufs_variant *variant = sunxi_variant(priv);
	uint32_t value;
	uint32_t ref_type;
	uint32_t ref_freq;
	bool external;

	if (!sunxi_variant_valid(variant))
		return UFSHC_ERR_INVALID;
	ref_type = sunxi_ref_clk_type(priv);
	ref_freq = sunxi_ref_clk_freq(variant, ref_type);
	external = sunxi_ref_clk_is_external(variant, ref_type);

	(void)base;
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

static int sunxi_phy_init(void *priv __attribute__((unused)))
{
	/* The PHY remains in reset until link_startup has programmed RMMI. */
	return 0;
}

static void sunxi_device_reset(uintptr_t base __attribute__((unused)), void *priv)
{
	const struct sunxi_ufs_variant *variant = sunxi_variant(priv);
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

static int sunxi_dme_write(struct ufshc_host *host, uint32_t attr, uint16_t selector, uint32_t value)
{
	return ufshc_dme_set_sel(host, attr, selector, value, false);
}

static int sunxi_dme_read(struct ufshc_host *host, uint32_t attr, uint16_t selector, uint32_t *value)
{
	return ufshc_dme_get_sel(host, attr, selector, value, false);
}

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

static int sunxi_phy_link_startup(struct ufshc_host *host, void *priv)
{
	const struct sunxi_ufs_variant *variant = sunxi_variant(priv);
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
		if (time_us() >= deadline)
			return UFSHC_ERR_TIMEOUT;
	}

	/* Exercise the indirect SRAM path as in the vendor sequence. */
	ret = sunxi_c10_read(host, TC_RAM_CMN0_B0_R0 + 2U, &scratch);
	if (ret)
		return ret;
	ret = sunxi_c10_write(host, TC_RAM_CMN0_B0_R0 + 2U, scratch);
	if (ret)
		return ret;
	if (cal.pll_rate_a || cal.pll_rate_b) {
		uint16_t coarse = cal.pll_rate_b ? cal.pll_rate_b : cal.pll_rate_a;

		ret = sunxi_c10_write(host, TC_MPLL_COARSE_TUNE, coarse);
		if (ret)
			return ret;
		ret = sunxi_c10_write(host, TC_MPLL_SKIPCAL_COARSE_TUNE, coarse);
		if (ret)
			return ret;
	}
	for (size_t i = 0; i < sizeof(c10) / sizeof(c10[0]); ++i) {
		ret = sunxi_c10_write(host, c10[i].reg, c10[i].value);
		if (ret)
			return ret;
	}
	if (cal.att_lane0 && cal.ctle_lane0 && cal.att_lane1 && cal.ctle_lane1) {
		afe_cal[0].value = cal.att_lane0;
		afe_cal[1].value = cal.att_lane1;
		afe_cal[2].value = cal.ctle_lane0;
		afe_cal[3].value = cal.ctle_lane1;
		for (size_t i = 0; i < sizeof(afe_cal) / sizeof(afe_cal[0]); ++i) {
			ret = sunxi_c10_write(host, afe_cal[i].reg, afe_cal[i].value);
			if (ret)
				return ret;
		}
	}
	if (!cal.pll_rate_a && !cal.pll_rate_b) {
		/* No fused PLL words: trigger the M-PHY automatic calibration. */
		ret = sunxi_c10_write(host, TC_MPLL_PWR_CTL_CAL_CTRL, 8);
		if (ret)
			return ret;
	}

	value = readl(variant->ufs_cfg_reg);
	value |= SUNXI_UFS_MPHY_SRAM_EXT_DONE;
	writel(value, variant->ufs_cfg_reg);
	ret = sunxi_dme_write(host, TC_VS_MPHYCFGUPDT, 0, 1);
	if (ret)
		return ret;
	ret = sunxi_dme_write(host, TC_VS_MPHY_DISABLE, 0, 0);
	if (ret)
		return ret;
	ret = sunxi_dme_write(host, TC_VS_DEBUG_SAVE_CONFIG_TIME, 0, 0x1b);
	if (ret)
		return ret;
	return sunxi_dme_write(host, TC_VS_CLK_MUX_SWITCHING_TIMER, 0, 0xa);
}

static int sunxi_link_up(struct ufshc_host *host, void *priv)
{
	uint32_t value;
	int ret;

	(void)priv;
	ret = sunxi_dme_read(host, TC_VS_POWERSTATE, 0, &value);
	if (ret || value != 2U)
		return ret ? ret : UFSHC_ERR_IO;
	ret = sunxi_dme_write(host, TC_T_CONNECTIONSTATE, 0, 0);
	if (ret)
		return ret;
	ret = sunxi_dme_write(host, TC_T_CPORTFLAGS, 0, 6);
	if (ret)
		return ret;
	ret = sunxi_dme_write(host, TC_T_CONNECTIONSTATE, 0, 1);
	if (ret)
		return ret;
	ret = sunxi_dme_read(host, TC_T_CONNECTIONSTATE, 0, &value);
	return ret ? ret : (value == 1U ? 0 : UFSHC_ERR_IO);
}

static void sunxi_disable(void *priv)
{
	const struct sunxi_ufs_variant *variant = sunxi_variant(priv);

	if (!sunxi_variant_valid(variant))
		return;
	/* Both core and PHY reset inputs are active low. */
	clrbits_le32(variant->reset_reg, variant->phy_rst);
	clrbits_le32(variant->reset_reg, variant->core_rst);
	sunxi_ref_clk(false, priv);
	sunxi_cfg_clk(priv);
	sunxi_ahb_clk(false, priv);
	sunxi_axi_clk(false, priv);
	*sunxi_ref_clk_cache_valid(priv) = false;
}

const struct ufshc_platform_ops sunxi_ufs_platform_ops = {
	.enable = sunxi_enable,
	.disable = sunxi_disable,
	.phy_init = sunxi_phy_init,
	.get_ref_clk_freq = sunxi_get_ref_clk_freq,
	.get_hs_rate = sunxi_get_hs_rate,
	.prepare = sunxi_prepare,
	.device_reset = sunxi_device_reset,
	.link_startup = sunxi_phy_link_startup,
	.link_up = sunxi_link_up,
	.priv = 0,
};
