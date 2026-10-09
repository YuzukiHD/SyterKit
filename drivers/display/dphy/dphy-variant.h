/* SPDX-License-Identifier: GPL-2.0+ */
#ifndef __DPHY_VARIANT_H__
#define __DPHY_VARIANT_H__

#include <stdint.h>

#include <drivers/display/display.h>

/*
 * Analog trim, PLL limits and lane timing that differ between phys of
 * different SoCs, selected by the "allwinner,variant" property.
 */
struct sunxi_dphy_variant {
	const char *name;
	sunxi_disp_modclk_t modclk; /* module clock layout */
	uint32_t ref_hz; /* PLL reference clock */
	uint32_t vco_min_hz;
	uint8_t max_lanes;
	uint8_t pll_m_max; /* post divider */
	uint8_t pll_div1_max; /* low speed divider */

	/* lane timing defaults, in lane byte clock periods */
	uint8_t lpx, hs_prepare, hs_trail;
	uint8_t clk_prepare, clk_zero, clk_pre, clk_post, clk_trail;
	uint8_t hs_delay, ulps_exit;

	/* analog trim, MIPI D-PHY mode */
	uint8_t hstx_ana; /* HSTX_ANA0 and 1 */
	uint8_t lptx_setc, lptx_setr;
	uint8_t ib, vres_set, vtt_set, vlptx_set, vlv_set;
	uint8_t hs_stop_dly;

	/* analog trim, LVDS mode */
	uint8_t lvds_vref1p6_single, lvds_vref1p6_dual, lvds_vref0p8;
	uint8_t lvds_ib;
};

/* chip descriptions live in drivers/display/platform/ */
extern const struct sunxi_dphy_variant sunxi_dphy_variant_sun252iw2;

#endif
