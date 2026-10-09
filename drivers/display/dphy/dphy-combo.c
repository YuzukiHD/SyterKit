/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Combo D-PHY: MIPI D-PHY transmitter for the DSI host and the LVDS
 * serialiser output stage, sharing one analog block and one PLL.
 */
#include <io.h>
#include <log.h>
#include <string.h>
#include <timer.h>

#include <drivers/display/display.h>

#include <dt2c/driver.h>

#include "dphy-regs.h"
#include "dphy-variant.h"

#define DPHY_ERR_RANGE (-5)

static const struct sunxi_dphy_variant *const dphy_variants[] = {
	&sunxi_dphy_variant_sun252iw2,
};

const struct sunxi_dphy_variant *sunxi_dphy_variant_lookup(const char *name)
{
	unsigned int i;

	if (name == NULL)
		return NULL;
	for (i = 0; i < sizeof(dphy_variants) / sizeof(dphy_variants[0]); i++)
		if (strcmp(name, dphy_variants[i]->name) == 0)
			return dphy_variants[i];
	return NULL;
}

static inline void dphy_write(const sunxi_dphy_t *p, uint32_t reg, uint32_t v)
{
	writel(v, p->res.base + reg);
}

static inline uint32_t dphy_read(const sunxi_dphy_t *p, uint32_t reg)
{
	return readl(p->res.base + reg);
}

static inline void dphy_update(const sunxi_dphy_t *p, uint32_t reg, uint32_t mask, uint32_t val)
{
	clrsetbits_le32(p->res.base + reg, mask, val);
}

/* ------------------------------------------------------------------ */
/* Power                                                               */
/* ------------------------------------------------------------------ */
int sunxi_dphy_power_on(sunxi_dphy_t *phy)
{
	if (phy == NULL || phy->var == NULL || phy->clk == NULL)
		return DRIVER_ERROR_INVALID;
	if (phy->powered)
		return 0;

	sunxi_disp_res_bus_enable(&phy->res, true);
	if (phy->res.mod_reg) {
		/* rate 0: pick the parent, bring up its PLL and open the gate, keep the divider */
		if (sunxi_disp_modclk_set(phy->clk, &phy->res, &phy->var->modclk, phy->res.mod_rate) == 0) {
			pr_err("dphy: module clock failed\n");
			sunxi_disp_res_bus_enable(&phy->res, false);
			return -4;
		}
	}
	if (phy->clk->soc != NULL && phy->clk->soc->dphy_calibrate != NULL)
		phy->clk->soc->dphy_calibrate();
	phy->powered = true;
	return 0;
}

void sunxi_dphy_power_off(sunxi_dphy_t *phy)
{
	if (phy == NULL || !phy->powered)
		return;
	if (phy->res.mod_reg)
		sunxi_disp_modclk_gate(&phy->res, false);
	sunxi_disp_res_bus_enable(&phy->res, false);
	phy->powered = false;
}

/* ------------------------------------------------------------------ */
/* LVDS                                                                */
/* ------------------------------------------------------------------ */
int sunxi_dphy_lvds_enable(sunxi_dphy_t *phy, bool dual_link)
{
	const struct sunxi_dphy_variant *v;

	if (phy == NULL || phy->var == NULL || !phy->powered)
		return DRIVER_ERROR_INVALID;
	v = phy->var;
	/* reference voltages of the LVDS driver */
	dphy_write(phy, COMBO_PHY_REG1,
		DISP_FIELD_PREP(COMBO_PHY_VREF1P6, dual_link ? v->lvds_vref1p6_dual : v->lvds_vref1p6_single) |
			DISP_FIELD_PREP(COMBO_PHY_VREF0P8, v->lvds_vref0p8));
	/* charge pump, then LVDS mode, then the LDO */
	dphy_write(phy, COMBO_PHY_REG0, COMBO_PHY_EN_CP);
	udelay(5);
	dphy_write(phy, COMBO_PHY_REG0, COMBO_PHY_EN_CP | COMBO_PHY_EN_LVDS);
	udelay(5);
	dphy_write(phy, COMBO_PHY_REG0, COMBO_PHY_EN_CP | COMBO_PHY_EN_LVDS | COMBO_PHY_EN_LDO);
	udelay(5);

	dphy_write(phy, DPHY_ANA4, DPHY_ANA4_EN_MIPI | DISP_FIELD_PREP(DPHY_ANA4_IB, v->lvds_ib));
	dphy_write(phy, DPHY_ANA3, DPHY_ANA3_ENLDOD | DPHY_ANA3_ENLDOR);
	dphy_write(phy, DPHY_ANA2, 0);
	dphy_write(phy, DPHY_ANA1, 0);
	return 0;
}

void sunxi_dphy_lvds_disable(sunxi_dphy_t *phy)
{
	if (phy == NULL || !phy->powered)
		return;
	dphy_write(phy, COMBO_PHY_REG1, 0);
	dphy_write(phy, COMBO_PHY_REG0, 0);
	dphy_write(phy, DPHY_ANA4, 0);
	dphy_write(phy, DPHY_ANA3, 0);
	dphy_write(phy, DPHY_ANA1, 0);
}

/* ------------------------------------------------------------------ */
/* MIPI D-PHY                                                          */
/* ------------------------------------------------------------------ */
struct dphy_pll_plan {
	uint32_t n, m, div0, div1;
};

/*
 * The PLL runs at the per-lane bit rate times the post divider m; the low
 * speed output (div0 * div1 = m * bits per lane per pixel) is the pixel clock
 * that drives the TCON in DSI mode. Only integer bits per lane per pixel can
 * be produced.
 */
static int dphy_pll_plan(
	const struct sunxi_dphy_variant *v, uint32_t pixclk_hz, uint32_t bpp, uint32_t lanes, struct dphy_pll_plan *p)
{
	uint32_t coef;
	uint64_t rate;

	if (!lanes || !pixclk_hz || bpp % lanes)
		return DRIVER_ERROR_INVALID;
	coef = bpp / lanes;
	rate = (uint64_t)pixclk_hz * coef;

	/* smallest post divider keeping the VCO in range */
	p->m = 1;
	while (rate * p->m < v->vco_min_hz && p->m < v->pll_m_max)
		p->m++;
	p->n = (uint32_t)(rate * p->m / v->ref_hz);
	p->div0 = (coef % 3 == 0) ? 3 : (coef % 2 == 0) ? 2 : 1;
	p->div1 = p->m * coef / p->div0;

	if (rate * p->m < v->vco_min_hz || p->div1 > v->pll_div1_max || p->n > DISP_FIELD_GET(DPHY_PLL0_N, DPHY_PLL0_N))
		return DPHY_ERR_RANGE;
	return 0;
}

/* program the PLL, returns the high speed clock */
static uint32_t dphy_set_pll(sunxi_dphy_t *phy, const struct dphy_pll_plan *p)
{
	dphy_update(
		phy, DPHY_PLL1, DPHY_PLL1_LS_GATING | DPHY_PLL1_HS_GATING, DPHY_PLL1_LS_GATING | DPHY_PLL1_HS_GATING);
	dphy_write(phy, DPHY_PLL2, 0); /* no spread spectrum */
	dphy_update(phy, DPHY_PLL0,
		DPHY_PLL0_N | DPHY_PLL0_P | DPHY_PLL0_M0 | DPHY_PLL0_M1 | DPHY_PLL0_LS_DIV0 | DPHY_PLL0_LS_DIV1 |
			DPHY_PLL0_PLL_EN | DPHY_PLL0_LDO_EN,
		DISP_FIELD_PREP(DPHY_PLL0_N, p->n) | DISP_FIELD_PREP(DPHY_PLL0_P, 0) |
			DISP_FIELD_PREP(DPHY_PLL0_M0, 0) | DISP_FIELD_PREP(DPHY_PLL0_M1, p->m - 1) |
			DISP_FIELD_PREP(DPHY_PLL0_LS_DIV0, p->div0 - 1) |
			DISP_FIELD_PREP(DPHY_PLL0_LS_DIV1, p->div1 - 1) | DPHY_PLL0_PLL_EN | DPHY_PLL0_LDO_EN);
	dphy_update(phy, DPHY_PLL1, DPHY_PLL1_LOCKDET_EN, DPHY_PLL1_LOCKDET_EN);
	dphy_update(phy, DPHY_PLL0, DPHY_PLL0_REG_UPDATE, DPHY_PLL0_REG_UPDATE);

	return (uint32_t)((uint64_t)phy->var->ref_hz * p->n / p->m);
}

static void dphy_set_timing(sunxi_dphy_t *phy, const sunxi_dphy_dsi_cfg_t *cfg)
{
	const struct sunxi_dphy_variant *v = phy->var;
	uint8_t hs_trail = cfg->hs_trail ? cfg->hs_trail : v->hs_trail;
	uint8_t clk_trail = cfg->clk_trail ? cfg->clk_trail : v->clk_trail;

	dphy_update(phy, DPHY_GCTL, DPHY_GCTL_MODULE_EN | DPHY_GCTL_LANE_NUM,
		DISP_FIELD_PREP(DPHY_GCTL_LANE_NUM, cfg->lanes - 1));
	dphy_update(phy, DPHY_TX_CTL, DPHY_TX_HSTX_CLK_CONT, DPHY_TX_HSTX_CLK_CONT);
	dphy_write(phy, DPHY_TX_TIME0,
		DISP_FIELD_PREP(DPHY_TX_LPX, v->lpx) | DISP_FIELD_PREP(DPHY_TX_HS_PRE, v->hs_prepare) |
			DISP_FIELD_PREP(DPHY_TX_HS_TRAIL, hs_trail));
	dphy_write(phy, DPHY_TX_TIME1,
		DISP_FIELD_PREP(DPHY_TX_CK_PREP, v->clk_prepare) | DISP_FIELD_PREP(DPHY_TX_CK_ZERO, v->clk_zero) |
			DISP_FIELD_PREP(DPHY_TX_CK_PRE, v->clk_pre) | DISP_FIELD_PREP(DPHY_TX_CK_POST, v->clk_post));
	dphy_write(phy, DPHY_TX_TIME2,
		DISP_FIELD_PREP(DPHY_TX_CK_TRAIL, clk_trail) | DISP_FIELD_PREP(DPHY_TX_HS_DLY, v->hs_delay));
	dphy_write(phy, DPHY_TX_TIME3, DISP_FIELD_PREP(DPHY_TX_ULPS_EXIT, v->ulps_exit));
	dphy_write(phy, DPHY_TX_TIME4,
		DISP_FIELD_PREP(DPHY_TX_HSTX_ANA0, v->hstx_ana) | DISP_FIELD_PREP(DPHY_TX_HSTX_ANA1, v->hstx_ana));
}

int sunxi_dphy_dsi_enable(sunxi_dphy_t *phy, const sunxi_dphy_dsi_cfg_t *cfg, uint32_t *hs_clk_hz)
{
	const struct sunxi_dphy_variant *v;
	struct dphy_pll_plan pll;
	uint32_t lanes_mask;
	int ret;

	if (phy == NULL || cfg == NULL || phy->var == NULL || !phy->powered)
		return DRIVER_ERROR_INVALID;
	v = phy->var;
	if (!cfg->lanes || cfg->lanes > v->max_lanes || !cfg->bpp)
		return DRIVER_ERROR_INVALID;
	ret = dphy_pll_plan(v, cfg->pixclk_hz, cfg->bpp, cfg->lanes, &pll);
	if (ret) {
		pr_err("dphy: no PLL setting for %u Hz, %u bpp on %u lane(s)\n", cfg->pixclk_hz, cfg->bpp, cfg->lanes);
		return ret;
	}
	lanes_mask = (1U << cfg->lanes) - 1;

	dphy_set_timing(phy, cfg);

	/* analog trim: bias, LP/HS drive levels, termination calibration */
	dphy_write(phy, DPHY_ANA4,
		DPHY_ANA4_EN_MIPI | DISP_FIELD_PREP(DPHY_ANA4_IB, v->ib) |
			DISP_FIELD_PREP(DPHY_ANA4_VRES_SET, v->vres_set) |
			DISP_FIELD_PREP(DPHY_ANA4_VTT_SET, v->vtt_set) |
			DISP_FIELD_PREP(DPHY_ANA4_VLPTX_SET, v->vlptx_set) |
			DISP_FIELD_PREP(DPHY_ANA4_VLV_SET, v->vlv_set) | DPHY_ANA4_EN_RESCAL);
	dphy_update(phy, DPHY_ANA2, DPHY_ANA2_ENCK_CPU | DPHY_ANA2_ENIB, DPHY_ANA2_ENCK_CPU | DPHY_ANA2_ENIB);
	dphy_update(phy, DPHY_ANA3, DPHY_ANA3_ENLDOR | DPHY_ANA3_ENLDOC | DPHY_ANA3_ENLDOD,
		DPHY_ANA3_ENLDOR | DPHY_ANA3_ENLDOC | DPHY_ANA3_ENLDOD);
	dphy_update(phy, DPHY_ANA0, DPHY_ANA0_LPTX_SETC | DPHY_ANA0_LPTX_SETR,
		DISP_FIELD_PREP(DPHY_ANA0_LPTX_SETC, v->lptx_setc) |
			DISP_FIELD_PREP(DPHY_ANA0_LPTX_SETR, v->lptx_setr));
	dphy_update(phy, COMBO_PHY_REG0, COMBO_PHY_EN_CP, COMBO_PHY_EN_CP);

	phy->hs_clk_hz = dphy_set_pll(phy, &pll);
	udelay(20);

	dphy_update(phy, COMBO_PHY_REG0, COMBO_PHY_EN_MIPI | COMBO_PHY_EN_LDO, COMBO_PHY_EN_MIPI | COMBO_PHY_EN_LDO);
	dphy_update(phy, COMBO_PHY_REG2, COMBO_PHY_HS_STOP_DLY, DISP_FIELD_PREP(COMBO_PHY_HS_STOP_DLY, v->hs_stop_dly));
	udelay(1);

	dphy_update(phy, DPHY_ANA3, DPHY_ANA3_ENVTTC | DPHY_ANA3_ENVTTD | DPHY_ANA3_ENDIV,
		DPHY_ANA3_ENVTTC | DPHY_ANA3_ENDIV | DISP_FIELD_PREP(DPHY_ANA3_ENVTTD, lanes_mask));
	dphy_update(phy, DPHY_ANA1, DPHY_ANA1_VTTMODE, DPHY_ANA1_VTTMODE);
	dphy_update(phy, DPHY_ANA2, DPHY_ANA2_ENP2S_CPU, DISP_FIELD_PREP(DPHY_ANA2_ENP2S_CPU, lanes_mask));
	dphy_update(phy, DPHY_GCTL, DPHY_GCTL_MODULE_EN, DPHY_GCTL_MODULE_EN);

	if (hs_clk_hz)
		*hs_clk_hz = phy->hs_clk_hz;
	pr_info("dphy: %u lane(s), HS clock %u Hz\n", cfg->lanes, phy->hs_clk_hz);
	return 0;
}

void sunxi_dphy_dsi_disable(sunxi_dphy_t *phy)
{
	if (phy == NULL || !phy->powered)
		return;
	dphy_update(phy, DPHY_ANA2, DPHY_ANA2_ENP2S_CPU, 0);
	dphy_update(phy, DPHY_ANA1, DPHY_ANA1_VTTMODE, 0);
	udelay(1);
	dphy_update(phy, DPHY_ANA2, DPHY_ANA2_ENCK_CPU, 0);
	udelay(1);
	dphy_update(phy, DPHY_ANA3, DPHY_ANA3_ENDIV, 0);
	udelay(1);
	dphy_update(phy, DPHY_ANA3, DPHY_ANA3_ENVTTD | DPHY_ANA3_ENVTTC, 0);
	udelay(1);
	dphy_update(phy, DPHY_ANA3, DPHY_ANA3_ENLDOD | DPHY_ANA3_ENLDOC | DPHY_ANA3_ENLDOR, 0);
	udelay(5);
	dphy_update(phy, DPHY_ANA2, DPHY_ANA2_ENIB, 0);
	dphy_write(phy, DPHY_ANA4, 0);
	dphy_write(phy, DPHY_ANA0, 0);
	dphy_update(phy, DPHY_ANA1, DPHY_ANA1_SVTT | DPHY_ANA1_CSMPS, 0);
	dphy_update(phy, DPHY_PLL0, DPHY_PLL0_PLL_EN, 0);
	dphy_write(phy, COMBO_PHY_REG0, 0);
	dphy_update(phy, DPHY_GCTL, DPHY_GCTL_MODULE_EN, 0);
}

void sunxi_dphy_dump(sunxi_dphy_t *phy)
{
	static const uint16_t regs[] = {
		DPHY_GCTL,
		DPHY_TX_CTL,
		DPHY_TX_TIME0,
		DPHY_TX_TIME1,
		DPHY_TX_TIME2,
		DPHY_ANA0,
		DPHY_ANA1,
		DPHY_ANA2,
		DPHY_ANA3,
		DPHY_ANA4,
		DPHY_PLL0,
		DPHY_PLL1,
		DPHY_PLL2,
		COMBO_PHY_REG0,
		COMBO_PHY_REG1,
		COMBO_PHY_REG2,
	};
	unsigned int i;

	if (phy == NULL)
		return;
	pr_debug("dphy%u: %s, hs clock %u Hz\n", phy->id, phy->powered ? "on" : "off", phy->hs_clk_hz);
	if (!phy->powered)
		return;
	for (i = 0; i < sizeof(regs) / sizeof(regs[0]); i++)
		pr_debug("  +%03x: %08x\n", regs[i], dphy_read(phy, regs[i]));
}

DT2C_DRIVER_COMPAT("allwinner,sunxi-combo-dphy");
