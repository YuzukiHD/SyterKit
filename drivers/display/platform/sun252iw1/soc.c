/* SPDX-License-Identifier: GPL-2.0+ */
#include <io.h>
#include <stdbool.h>
#include <stdint.h>

#include <drivers/display/display.h>
#include <drivers/display/sun252iw1.h>

#include "../sunxi-pll.h"

#define CCU_BASE	      0x02001000U
#define CCU_MBUS_SW_CFG_GATE  (CCU_BASE + 0x804U) /* bit 19: video out */
#define CCU_AHB_SW_CFG_GATE   (CCU_BASE + 0xe04U) /* bit 3: video out */

/*
 * sun252iw1 (V861/V881): PLL_VIDEO and PLL_PERI are N = field + 1 multipliers of the 24 MHz oscillator
 * (1272..2520 MHz). PLL_VIDEO(4X) = PLL / (M + 1) with M in bit 0, which reads like the "reference / 2" bit
 * of the other chips. PLL_PERI(2X) = PLL / (M + 1) with M in bits 18:16, there is no reference divider.
 */

#define HOSC_HZ 24000000U

/* the display node lists the PLL registers in this order (allwinner,pll-video, allwinner,pll-peri) */
enum { PLL_VIDEO, PLL_PERI };

static const struct sunxi_pll_nm pll_video = {
	.n_shift = 8, .n_width = 8, .n_bias = 1,
	.m_bit = 0,
	.en_bit = 31, .ldo_bit = 30, .lock_en_bit = 29, .locked_bit = 28, .out_bit = 27,
	.min_hz = 1272000000U, .max_hz = 2520000000U,
};

static const struct sunxi_pll_nm pll_peri = {
	.n_shift = 8, .n_width = 8, .n_bias = 1,
	.m_bit = SUNXI_PLL_NO_BIT,
	.p_shift = 16, .p_width = 3,
	.en_bit = SUNXI_PLL_NO_BIT, .ldo_bit = SUNXI_PLL_NO_BIT, .lock_en_bit = SUNXI_PLL_NO_BIT,
	.locked_bit = SUNXI_PLL_NO_BIT, .out_bit = SUNXI_PLL_NO_BIT,
};

static bool src_is_video(sunxi_disp_src_t src)
{
	return src == SUN252IW1_DISP_SRC_PLL_VIDEO_1X || src == SUN252IW1_DISP_SRC_PLL_VIDEO_4X;
}

static uint32_t sun252iw1_src_rate(const sunxi_disp_clk_t *clk, sunxi_disp_src_t src)
{
	switch (src) {
	case SUN252IW1_DISP_SRC_HOSC:
		return HOSC_HZ;
	case SUN252IW1_DISP_SRC_PLL_PERI_2X:
		return sunxi_pll_nm_rate(clk->pll_reg[PLL_PERI], HOSC_HZ, &pll_peri);
	case SUN252IW1_DISP_SRC_PLL_PERI_300M:
		return sunxi_pll_nm_rate(clk->pll_reg[PLL_PERI], HOSC_HZ, &pll_peri) / 4U;
	case SUN252IW1_DISP_SRC_PLL_VIDEO_4X:
		return sunxi_pll_nm_rate(clk->pll_reg[PLL_VIDEO], HOSC_HZ, &pll_video);
	case SUN252IW1_DISP_SRC_PLL_VIDEO_1X:
		return sunxi_pll_nm_rate(clk->pll_reg[PLL_VIDEO], HOSC_HZ, &pll_video) / 4U;
	default:
		return 0;
	}
}

/* PLL_VIDEO sources can only be made in multiples of the oscillator, the others are divided from a fixed parent */
static uint32_t sun252iw1_src_round(const sunxi_disp_clk_t *clk, sunxi_disp_src_t src, uint32_t want)
{
	uint32_t mult, hosc_mult, parent, div;

	if (src_is_video(src)) {
		mult = src == SUN252IW1_DISP_SRC_PLL_VIDEO_1X ? 4U : 1U;
		hosc_mult = (want * mult + HOSC_HZ / 2) / HOSC_HZ;
		if (hosc_mult == 0)
			hosc_mult = 1;
		return hosc_mult * HOSC_HZ / mult;
	}
	parent = sun252iw1_src_rate(clk, src);
	if (!parent)
		return want;
	div = (parent + want / 2) / want;
	if (div == 0)
		div = 1;
	return parent / div;
}

/* PLL_VIDEO is dedicated to the display clock that asks for a rate: it is retuned to it. PLL_PERI belongs to the system. */
static int sun252iw1_src_prepare(const sunxi_disp_clk_t *clk, sunxi_disp_src_t src, uint32_t rate_hz)
{
	if (!src_is_video(src))
		return 0;
	if (!rate_hz) {
		sunxi_pll_nm_enable(clk->pll_reg[PLL_VIDEO], &pll_video, true);
		return 0;
	}
	sunxi_pll_nm_set(clk->pll_reg[PLL_VIDEO], HOSC_HZ, &pll_video,
		src == SUN252IW1_DISP_SRC_PLL_VIDEO_1X ? rate_hz * 4U : rate_hz);
	sunxi_pll_nm_enable(clk->pll_reg[PLL_VIDEO], &pll_video, true);
	return 1;
}

/*
 * The vendor driver ungates the video output register buses as optional "PPU" clocks. Both are already on in
 * FEL after reset, so this only matters when something has turned them off.
 */
static void sun252iw1_soc_init(void)
{
	setbits_le32(CCU_AHB_SW_CFG_GATE, 1U << 3);
	setbits_le32(CCU_MBUS_SW_CFG_GATE, 1U << 19);
}

const struct sunxi_disp_soc sunxi_disp_soc_sun252iw1 = {
	.compatible = "sun252iw1",
	.src_rate = sun252iw1_src_rate,
	.src_round = sun252iw1_src_round,
	.src_prepare = sun252iw1_src_prepare,
	.soc_init = sun252iw1_soc_init,
};
