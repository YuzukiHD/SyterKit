/* SPDX-License-Identifier: GPL-2.0+ */
#include <io.h>
#include <stdint.h>

#include <stdbool.h>

#include <drivers/display/display.h>
#include <drivers/display/sun252iw2.h>

#include "../sunxi-pll.h"

#define SYSCTRL_BASE 0x03000000U
#define SID_BASE     0x03006000U

/*
 * The DSI/LVDS pad termination is trimmed per chip: a 4 bit code is fused in
 * SID word 0x18 bits [7:4] and has to be copied, with an unlock key, into the
 * system control resistor register.
 */
static void sun252iw2_dphy_calibrate(void)
{
	uint32_t code = readl(SID_BASE + 0x18);

	if (!code)
		return;
	code = (code >> 4) & 0xf;
	writel(code | (code << 8) | (0x1937U << 16), SYSCTRL_BASE + 0x164);
}

/*
 * Gives the SRAM from 0x2b000 up to the display engine (its register file lives there). The CPU loses that part, so an
 * image running from the SRAM has to keep its code, data, bss and stack below 0x2b000.
 */
static void sun252iw2_soc_init(void)
{
	writel(0x0b000000, SYSCTRL_BASE + 0x04);
}


#define HOSC_HZ 24000000U

/* the display node lists the PLL registers in this order (allwinner,pll-video, allwinner,pll-peri) */
enum { PLL_VIDEO, PLL_PERI };

static const struct sunxi_pll_nm pll_video = {
	.n_shift = 8, .n_width = 8, .n_bias = 1,
	.m_bit = 1,
	.en_bit = 31, .ldo_bit = 30, .lock_en_bit = 29, .locked_bit = 28, .out_bit = 27,
	.min_hz = 288000000U, .max_hz = 2400000000U,
};

static const struct sunxi_pll_nm pll_peri = {
	.n_shift = 8, .n_width = 8, .n_bias = 1,
	.m_bit = 1,
	.p_shift = 16, .p_width = 3,
	.en_bit = SUNXI_PLL_NO_BIT, .ldo_bit = SUNXI_PLL_NO_BIT, .lock_en_bit = SUNXI_PLL_NO_BIT,
	.locked_bit = SUNXI_PLL_NO_BIT, .out_bit = SUNXI_PLL_NO_BIT,
};

static bool src_is_video(sunxi_disp_src_t src)
{
	return src == SUN252IW2_DISP_SRC_PLL_VIDEO_1X || src == SUN252IW2_DISP_SRC_PLL_VIDEO_4X;
}

static uint32_t sun252iw2_src_rate(const sunxi_disp_clk_t *clk, sunxi_disp_src_t src)
{
	switch (src) {
	case SUN252IW2_DISP_SRC_HOSC:
		return HOSC_HZ;
	case SUN252IW2_DISP_SRC_PLL_PERI_2X:
		return sunxi_pll_nm_rate(clk->pll_reg[PLL_PERI], HOSC_HZ, &pll_peri);
	case SUN252IW2_DISP_SRC_PLL_PERI_1X:
		return sunxi_pll_nm_rate(clk->pll_reg[PLL_PERI], HOSC_HZ, &pll_peri) / 2U;
	case SUN252IW2_DISP_SRC_PLL_VIDEO_4X:
		return sunxi_pll_nm_rate(clk->pll_reg[PLL_VIDEO], HOSC_HZ, &pll_video);
	case SUN252IW2_DISP_SRC_PLL_VIDEO_1X:
		return sunxi_pll_nm_rate(clk->pll_reg[PLL_VIDEO], HOSC_HZ, &pll_video) / 4U;
	default:
		return 0;
	}
}

/* PLL_VIDEO sources can only be made in multiples of the oscillator, the others are divided from a fixed parent */
static uint32_t sun252iw2_src_round(const sunxi_disp_clk_t *clk, sunxi_disp_src_t src, uint32_t want)
{
	uint32_t mult, hosc_mult, parent, div;

	if (src_is_video(src)) {
		mult = src == SUN252IW2_DISP_SRC_PLL_VIDEO_1X ? 4U : 1U;
		hosc_mult = (want * mult + HOSC_HZ / 2) / HOSC_HZ;
		if (hosc_mult == 0)
			hosc_mult = 1;
		return hosc_mult * HOSC_HZ / mult;
	}
	parent = sun252iw2_src_rate(clk, src);
	if (!parent)
		return want;
	div = (parent + want / 2) / want;
	if (div == 0)
		div = 1;
	return parent / div;
}

/* PLL_VIDEO is dedicated to the display clock that asks for a rate: it is retuned to it. PLL_PERI belongs to the system. */
static int sun252iw2_src_prepare(const sunxi_disp_clk_t *clk, sunxi_disp_src_t src, uint32_t rate_hz)
{
	if (!src_is_video(src))
		return 0;
	if (!rate_hz) {
		sunxi_pll_nm_enable(clk->pll_reg[PLL_VIDEO], &pll_video, true);
		return 0;
	}
	sunxi_pll_nm_set(clk->pll_reg[PLL_VIDEO], HOSC_HZ, &pll_video,
		src == SUN252IW2_DISP_SRC_PLL_VIDEO_1X ? rate_hz * 4U : rate_hz);
	sunxi_pll_nm_enable(clk->pll_reg[PLL_VIDEO], &pll_video, true);
	return 1;
}

const struct sunxi_disp_soc sunxi_disp_soc_sun252iw2 = {
	.compatible = "sun252iw2",
	.src_rate = sun252iw2_src_rate,
	.src_round = sun252iw2_src_round,
	.src_prepare = sun252iw2_src_prepare,
	.soc_init = sun252iw2_soc_init,
	.dphy_calibrate = sun252iw2_dphy_calibrate,
};
