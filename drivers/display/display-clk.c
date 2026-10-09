/* SPDX-License-Identifier: GPL-2.0+ */
#define pr_fmt(fmt) "disp-clk: " fmt

/*
 * Clock helpers of the display blocks: bus gate and reset bits, module clock
 * registers (gate bit 31, parent select 26:24, linear or M/P divider) and the
 * shared PLL_VIDEO / PLL_PERI. Every register address comes from the device
 * tree; the field positions come from struct sunxi_disp_soc.
 */

#include <io.h>
#include <stdbool.h>
#include <stdint.h>
#include <types.h>

#include <timer.h>

#include <common.h>
#include <log.h>

#include <drivers/display/display.h>

#define MODCLK_GATE	 (1U << 31)
#define MODCLK_MUX_SHIFT 24
#define MODCLK_MUX_MASK	 (7U << MODCLK_MUX_SHIFT)
#define MODCLK_P_SHIFT	 8
#define MODCLK_P_MASK	 (3U << MODCLK_P_SHIFT)
#define MODCLK_M_MASK	 0xfU

static inline uint32_t field_mask(uint8_t shift, uint8_t width)
{
	return (width >= 32U ? 0xffffffffU : ((1U << width) - 1U)) << shift;
}

static inline bool bit_valid(uint8_t bit)
{
	return bit != 0xffU;
}

uint32_t sunxi_disp_src_rate(const sunxi_disp_clk_t *clk, sunxi_disp_src_t src)
{
	const struct sunxi_disp_soc *soc = clk->soc;
	uint32_t reg, n, post_div0, rate;

	switch (src) {
	case SUNXI_DISP_SRC_HOSC:
		return soc->hosc_hz;
	case SUNXI_DISP_SRC_PLL_PERI_1X:
	case SUNXI_DISP_SRC_PLL_PERI_2X:
		reg = readl(clk->pll_peri_reg);
		n = ((reg & field_mask(soc->pll_peri_n_shift, soc->pll_peri_n_width)) >> soc->pll_peri_n_shift) +
		    soc->pll_peri_n_bias;
		post_div0 =
			((reg & field_mask(soc->pll_peri_p0_shift, soc->pll_peri_p0_width)) >> soc->pll_peri_p0_shift) +
			1;
		rate = soc->hosc_hz;
		if (bit_valid(soc->pll_peri_m_bit) && (reg & (1U << soc->pll_peri_m_bit)))
			rate /= 2;
		rate = rate / post_div0 * n;
		return src == SUNXI_DISP_SRC_PLL_PERI_1X ? rate / 2 : rate;
	case SUNXI_DISP_SRC_PLL_VIDEO_4X:
	case SUNXI_DISP_SRC_PLL_VIDEO_1X:
		reg = readl(clk->pll_video_reg);
		n = ((reg & field_mask(soc->pll_video_n_shift, soc->pll_video_n_width)) >> soc->pll_video_n_shift) +
		    soc->pll_video_n_bias;
		rate = soc->hosc_hz;
		if (bit_valid(soc->pll_video_m_bit) && (reg & (1U << soc->pll_video_m_bit)))
			rate /= 2;
		rate *= n;
		return src == SUNXI_DISP_SRC_PLL_VIDEO_1X ? rate / soc->pll_video_4x_div : rate;
	default:
		return 0;
	}
}

static bool src_is_video(sunxi_disp_src_t src)
{
	return src == SUNXI_DISP_SRC_PLL_VIDEO_1X || src == SUNXI_DISP_SRC_PLL_VIDEO_4X;
}

void sunxi_disp_pll_video_enable(const sunxi_disp_clk_t *clk, bool on)
{
	const struct sunxi_disp_soc *soc = clk->soc;
	uintptr_t reg = clk->pll_video_reg;
	int tries = 10000;

	if (!on) {
		if (bit_valid(soc->pll_video_out_bit))
			clrbits_le32(reg, 1U << soc->pll_video_out_bit);
		clrbits_le32(reg, 1U << soc->pll_video_en_bit);
		if (bit_valid(soc->pll_video_ldo_bit))
			clrbits_le32(reg, 1U << soc->pll_video_ldo_bit);
		return;
	}

	if ((readl(reg) & (1U << soc->pll_video_en_bit)) &&
		(!bit_valid(soc->pll_video_out_bit) || (readl(reg) & (1U << soc->pll_video_out_bit))))
		return;

	if (bit_valid(soc->pll_video_ldo_bit))
		setbits_le32(reg, 1U << soc->pll_video_ldo_bit);
	setbits_le32(reg, 1U << soc->pll_video_en_bit);
	if (bit_valid(soc->pll_video_lock_en_bit))
		setbits_le32(reg, 1U << soc->pll_video_lock_en_bit);
	if (bit_valid(soc->pll_video_locked_bit)) {
		while (tries-- && !(readl(reg) & (1U << soc->pll_video_locked_bit)))
			udelay(10);
		if (tries < 0)
			pr_warn("PLL_VIDEO did not lock\n");
	} else {
		udelay(100);
	}
	if (bit_valid(soc->pll_video_out_bit))
		setbits_le32(reg, 1U << soc->pll_video_out_bit);
}

/* PLL_VIDEO = hosc * N, the input divider is cleared so that the rate is exact */
int sunxi_disp_pll_video_set(const sunxi_disp_clk_t *clk, uint32_t hz)
{
	const struct sunxi_disp_soc *soc = clk->soc;
	uintptr_t reg = clk->pll_video_reg;
	uint32_t n, nmax, was_on;

	if (hz < soc->pll_video_min_hz)
		hz = soc->pll_video_min_hz;
	if (hz > soc->pll_video_max_hz)
		hz = soc->pll_video_max_hz;
	n = (hz + soc->hosc_hz / 2) / soc->hosc_hz;
	nmax = (1U << soc->pll_video_n_width) - 1U + soc->pll_video_n_bias;
	if (n < soc->pll_video_n_bias)
		n = soc->pll_video_n_bias;
	if (n > nmax)
		n = nmax;

	was_on = readl(reg) & (1U << soc->pll_video_en_bit);
	if (was_on)
		sunxi_disp_pll_video_enable(clk, false);
	if (bit_valid(soc->pll_video_m_bit))
		clrbits_le32(reg, 1U << soc->pll_video_m_bit);
	clrsetbits_le32(reg, field_mask(soc->pll_video_n_shift, soc->pll_video_n_width),
		(n - soc->pll_video_n_bias) << soc->pll_video_n_shift);
	if (was_on)
		sunxi_disp_pll_video_enable(clk, true);
	return 0;
}

void sunxi_disp_res_bus_enable(const sunxi_disp_res_t *res, bool on)
{
	if (on) {
		if (res->reset.reg)
			setbits_le32(res->reset.reg, 1U << res->reset.bit);
		if (res->gate.reg)
			setbits_le32(res->gate.reg, 1U << res->gate.bit);
	} else {
		if (res->gate.reg)
			clrbits_le32(res->gate.reg, 1U << res->gate.bit);
		if (res->reset.reg)
			clrbits_le32(res->reset.reg, 1U << res->reset.bit);
	}
}

void sunxi_disp_modclk_gate(const sunxi_disp_res_t *res, bool on)
{
	if (!res->mod_reg)
		return;
	if (on)
		setbits_le32(res->mod_reg, MODCLK_GATE);
	else
		clrbits_le32(res->mod_reg, MODCLK_GATE);
}

static sunxi_disp_src_t modclk_src(const sunxi_disp_res_t *res, const sunxi_disp_modclk_t *desc)
{
	return res->mod_mux < SUNXI_DISP_MAX_PARENTS ? desc->parents[res->mod_mux] : SUNXI_DISP_SRC_NONE;
}

uint32_t sunxi_disp_modclk_get(
	const sunxi_disp_clk_t *clk, const sunxi_disp_res_t *res, const sunxi_disp_modclk_t *desc)
{
	uint32_t reg, mux, rate;

	if (!res->mod_reg)
		return 0;
	reg = readl(res->mod_reg);
	mux = (reg & MODCLK_MUX_MASK) >> MODCLK_MUX_SHIFT;
	rate = sunxi_disp_src_rate(clk, mux < SUNXI_DISP_MAX_PARENTS ? desc->parents[mux] : SUNXI_DISP_SRC_NONE);
	if (desc->div == SUNXI_DISP_DIV_MP)
		return rate / ((reg & MODCLK_M_MASK) + 1) >> ((reg & MODCLK_P_MASK) >> MODCLK_P_SHIFT);
	return rate / ((reg & field_mask(0, desc->div_width)) + 1);
}

uint32_t sunxi_disp_modclk_set(
	const sunxi_disp_clk_t *clk, const sunxi_disp_res_t *res, const sunxi_disp_modclk_t *desc, uint32_t rate_hz)
{
	sunxi_disp_src_t src = modclk_src(res, desc);
	uint32_t parent, div, maxdiv;

	if (!res->mod_reg || src == SUNXI_DISP_SRC_NONE)
		return 0;

	if (res->mod_div) {
		/* fixed divider: the parent (and its PLL) is somebody else's business */
		if (src_is_video(src))
			sunxi_disp_pll_video_enable(clk, true);
		if (desc->div == SUNXI_DISP_DIV_MP)
			clrsetbits_le32(res->mod_reg, MODCLK_M_MASK | MODCLK_P_MASK, res->mod_div - 1U);
		else
			clrsetbits_le32(res->mod_reg, field_mask(0, desc->div_width), res->mod_div - 1U);
		clrsetbits_le32(res->mod_reg, MODCLK_MUX_MASK, (uint32_t)res->mod_mux << MODCLK_MUX_SHIFT);
		setbits_le32(res->mod_reg, MODCLK_GATE);
		return sunxi_disp_modclk_get(clk, res, desc);
	}

	if (!rate_hz) {
		/* no rate wanted: select the parent, keep the divider, run the clock */
		if (src_is_video(src))
			sunxi_disp_pll_video_enable(clk, true);
		clrsetbits_le32(res->mod_reg, MODCLK_MUX_MASK, (uint32_t)res->mod_mux << MODCLK_MUX_SHIFT);
		setbits_le32(res->mod_reg, MODCLK_GATE);
		return sunxi_disp_modclk_get(clk, res, desc);
	}

	if (src_is_video(src)) {
		/* the video PLL is dedicated to this clock: retune it, divider 1 */
		uint32_t pll = src == SUNXI_DISP_SRC_PLL_VIDEO_1X ? rate_hz * clk->soc->pll_video_4x_div : rate_hz;

		sunxi_disp_pll_video_set(clk, pll);
		sunxi_disp_pll_video_enable(clk, true);
		div = 1;
	} else {
		parent = sunxi_disp_src_rate(clk, src);
		if (!parent)
			return 0;
		maxdiv = desc->div == SUNXI_DISP_DIV_MP ? 16U : (1U << desc->div_width);
		div = (parent + rate_hz / 2) / rate_hz;
		if (div < 1U)
			div = 1U;
		if (div > maxdiv)
			div = maxdiv;
	}

	if (desc->div == SUNXI_DISP_DIV_MP)
		clrsetbits_le32(res->mod_reg, MODCLK_M_MASK | MODCLK_P_MASK, div - 1U);
	else
		clrsetbits_le32(res->mod_reg, field_mask(0, desc->div_width), div - 1U);
	clrsetbits_le32(res->mod_reg, MODCLK_MUX_MASK, (uint32_t)res->mod_mux << MODCLK_MUX_SHIFT);
	setbits_le32(res->mod_reg, MODCLK_GATE);
	return sunxi_disp_modclk_get(clk, res, desc);
}
