/* SPDX-License-Identifier: GPL-2.0+ */
#include "../../dphy/dphy-variant.h"

/* sun252iw2 DPHY */
const struct sunxi_dphy_variant sunxi_dphy_variant_sun252iw2 = {
	.name = "sun252iw2",
	.modclk = {
		.div = SUNXI_DISP_DIV_LINEAR,
		.div_width = 5,
		.parents = { SUNXI_DISP_SRC_PLL_VIDEO_4X, SUNXI_DISP_SRC_PLL_PERI_2X },
	},
	.ref_hz = 24000000U,
	.vco_min_hz = 1100000000U,
	.max_lanes = 4,
	.pll_m_max = 16,
	.pll_div1_max = 16,
	.lpx = 14, .hs_prepare = 6, .hs_trail = 4,
	.clk_prepare = 7, .clk_zero = 50, .clk_pre = 3, .clk_post = 10,
	.clk_trail = 30, .hs_delay = 10, .ulps_exit = 3,
	.hstx_ana = 3,
	.lptx_setc = 7, .lptx_setr = 7,
	.ib = 4, .vres_set = 3, .vtt_set = 1, .vlptx_set = 3, .vlv_set = 4,
	.hs_stop_dly = 20,
	.lvds_vref1p6_single = 5, .lvds_vref1p6_dual = 6, .lvds_vref0p8 = 3,
	.lvds_ib = 4,
};
