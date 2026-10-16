/* SPDX-License-Identifier: GPL-2.0+ */
#define pr_fmt(fmt) "disp-clk: " fmt

/*
 * Clock helpers of the display blocks: bus gate and reset bits and module clock registers (gate bit 31, parent
 * select 26:24, linear or M/P divider). Every register address comes from the device tree. How a parent is
 * built (which PLL, what it has to be set to) is chip knowledge and is asked from struct sunxi_disp_soc.
 */

#include <io.h>
#include <stdbool.h>
#include <stdint.h>
#include <types.h>

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

uint32_t sunxi_disp_src_rate(const sunxi_disp_clk_t *clk, sunxi_disp_src_t src)
{
	if (clk == NULL || clk->soc == NULL || clk->soc->src_rate == NULL)
		return 0;
	return clk->soc->src_rate(clk, src);
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

static void modclk_write_div(const sunxi_disp_res_t *res, const sunxi_disp_modclk_t *desc, uint32_t div)
{
	if (desc->div == SUNXI_DISP_DIV_MP)
		clrsetbits_le32(res->mod_reg, MODCLK_M_MASK | MODCLK_P_MASK, div - 1U);
	else
		clrsetbits_le32(res->mod_reg, field_mask(0, desc->div_width), div - 1U);
}

uint32_t sunxi_disp_modclk_set(
	const sunxi_disp_clk_t *clk, const sunxi_disp_res_t *res, const sunxi_disp_modclk_t *desc, uint32_t rate_hz)
{
	const struct sunxi_disp_soc *soc = clk->soc;
	sunxi_disp_src_t src = modclk_src(res, desc);
	uint32_t parent, div, maxdiv;
	int own;

	if (!res->mod_reg || src == SUNXI_DISP_SRC_NONE || soc == NULL || soc->src_prepare == NULL)
		return 0;

	if (res->mod_div || !rate_hz) {
		/*
		 * Fixed divider, or no rate wanted: the source keeps its rate (somebody else owns it), it only has to
		 * run. The divider is left alone when no rate was asked for.
		 */
		if (soc->src_prepare(clk, src, 0) < 0)
			return 0;
		if (res->mod_div)
			modclk_write_div(res, desc, res->mod_div);
		clrsetbits_le32(res->mod_reg, MODCLK_MUX_MASK, (uint32_t)res->mod_mux << MODCLK_MUX_SHIFT);
		setbits_le32(res->mod_reg, MODCLK_GATE);
		return sunxi_disp_modclk_get(clk, res, desc);
	}

	own = soc->src_prepare(clk, src, rate_hz);
	if (own < 0)
		return 0;
	if (own) {
		/* the source was set to the module rate: divider 1 */
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

	modclk_write_div(res, desc, div);
	clrsetbits_le32(res->mod_reg, MODCLK_MUX_MASK, (uint32_t)res->mod_mux << MODCLK_MUX_SHIFT);
	setbits_le32(res->mod_reg, MODCLK_GATE);
	return sunxi_disp_modclk_get(clk, res, desc);
}
