/* SPDX-License-Identifier: GPL-2.0+ */
#define pr_fmt(fmt) "disp-pll: " fmt

#include <io.h>
#include <stdbool.h>
#include <stdint.h>
#include <types.h>

#include <timer.h>

#include <common.h>
#include <log.h>

#include "sunxi-pll.h"

static inline uint32_t field_mask(uint8_t shift, uint8_t width)
{
	return (width >= 32U ? 0xffffffffU : ((1U << width) - 1U)) << shift;
}

static inline bool bit_valid(uint8_t bit)
{
	return bit != SUNXI_PLL_NO_BIT;
}

uint32_t sunxi_pll_nm_rate(uintptr_t reg_addr, uint32_t hosc_hz, const struct sunxi_pll_nm *pll)
{
	uint32_t reg = readl(reg_addr), rate = hosc_hz, post = 1U;
	uint32_t n = ((reg & field_mask(pll->n_shift, pll->n_width)) >> pll->n_shift) + pll->n_bias;

	if (pll->p_width)
		post = ((reg & field_mask(pll->p_shift, pll->p_width)) >> pll->p_shift) + 1U;
	if (bit_valid(pll->m_bit) && (reg & (1U << pll->m_bit)))
		rate /= 2;
	return rate / post * n;
}

void sunxi_pll_nm_enable(uintptr_t reg, const struct sunxi_pll_nm *pll, bool on)
{
	int tries = 10000;

	if (!on) {
		if (bit_valid(pll->out_bit))
			clrbits_le32(reg, 1U << pll->out_bit);
		clrbits_le32(reg, 1U << pll->en_bit);
		if (bit_valid(pll->ldo_bit))
			clrbits_le32(reg, 1U << pll->ldo_bit);
		return;
	}

	if ((readl(reg) & (1U << pll->en_bit)) &&
		(!bit_valid(pll->out_bit) || (readl(reg) & (1U << pll->out_bit))))
		return;

	if (bit_valid(pll->ldo_bit))
		setbits_le32(reg, 1U << pll->ldo_bit);
	setbits_le32(reg, 1U << pll->en_bit);
	if (bit_valid(pll->lock_en_bit))
		setbits_le32(reg, 1U << pll->lock_en_bit);
	if (bit_valid(pll->locked_bit)) {
		while (tries-- && !(readl(reg) & (1U << pll->locked_bit)))
			udelay(10);
		if (tries < 0)
			pr_warn("PLL did not lock\n");
	} else {
		udelay(100);
	}
	if (bit_valid(pll->out_bit))
		setbits_le32(reg, 1U << pll->out_bit);
}

int sunxi_pll_nm_set(uintptr_t reg, uint32_t hosc_hz, const struct sunxi_pll_nm *pll, uint32_t hz)
{
	uint32_t n, nmax, was_on;

	if (hz < pll->min_hz)
		hz = pll->min_hz;
	if (hz > pll->max_hz)
		hz = pll->max_hz;
	n = (hz + hosc_hz / 2) / hosc_hz;
	nmax = (1U << pll->n_width) - 1U + pll->n_bias;
	if (n < pll->n_bias)
		n = pll->n_bias;
	if (n > nmax)
		n = nmax;

	was_on = readl(reg) & (1U << pll->en_bit);
	if (was_on)
		sunxi_pll_nm_enable(reg, pll, false);
	if (bit_valid(pll->m_bit))
		clrbits_le32(reg, 1U << pll->m_bit);
	clrsetbits_le32(reg, field_mask(pll->n_shift, pll->n_width), (n - pll->n_bias) << pll->n_shift);
	if (was_on)
		sunxi_pll_nm_enable(reg, pll, true);
	return 0;
}
