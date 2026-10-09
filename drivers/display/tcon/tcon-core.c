/* SPDX-License-Identifier: GPL-2.0+ */
#define pr_fmt(fmt) "tcon: " fmt

/*
 * TCON LCD timing controller: parallel RGB (HV), LVDS and MIPI DSI (trigger /
 * CPU interface) output timing. Register offsets, divider ranges and the
 * clock source layout come from struct sunxi_tcon_variant.
 */

#include <io.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <types.h>

#include <timer.h>

#include <common.h>
#include <dt2c/driver.h>
#include <log.h>

#include "tcon-priv.h"
#include "tcon-regs.h"

extern const struct sunxi_tcon_variant sunxi_tcon_variant_sun252iw2;

static const struct sunxi_tcon_variant *const tcon_variants[] = {
	&sunxi_tcon_variant_sun252iw2,
};

const struct sunxi_tcon_variant *sunxi_tcon_find_variant(const char *name)
{
	unsigned int i;

	for (i = 0; name != NULL && i < sizeof(tcon_variants) / sizeof(tcon_variants[0]); i++)
		if (strcmp(tcon_variants[i]->name, name) == 0)
			return tcon_variants[i];
	return NULL;
}

static uintptr_t tcon_reg_address(const sunxi_tcon_t *tcon, uint16_t off)
{
	return tcon->res.base + off;
}

static uint32_t tcon_read_reg(const sunxi_tcon_t *tcon, uint16_t off)
{
	return readl(tcon_reg_address(tcon, off));
}

static void tcon_write_reg(const sunxi_tcon_t *tcon, uint16_t off, uint32_t val)
{
	writel(val, tcon_reg_address(tcon, off));
}

static void tcon_modify_reg(const sunxi_tcon_t *tcon, uint16_t off, uint32_t mask, uint32_t val)
{
	clrsetbits_le32(tcon_reg_address(tcon, off), mask, val);
}

static uint32_t abs_diff(uint32_t lhs, uint32_t rhs)
{
	return lhs > rhs ? lhs - rhs : rhs - lhs;
}

/* ------------------------------------------------------------------ */
/* Pixel clock                                                         */
/* ------------------------------------------------------------------ */
static bool src_is_video(sunxi_disp_src_t src)
{
	return src == SUNXI_DISP_SRC_PLL_VIDEO_1X || src == SUNXI_DISP_SRC_PLL_VIDEO_4X;
}

/* the module rate that can really be made for a wanted one */
static uint32_t mod_round(const sunxi_tcon_t *tcon, uint32_t want)
{
	const struct sunxi_disp_soc *soc = tcon->clk->soc;
	sunxi_disp_src_t src = tcon->res.mod_mux < SUNXI_DISP_MAX_PARENTS ?
				       tcon->var->modclk.parents[tcon->res.mod_mux] :
				       SUNXI_DISP_SRC_NONE;
	uint32_t mult, hosc_mult, parent, div;

	if (src_is_video(src)) {
		mult = src == SUNXI_DISP_SRC_PLL_VIDEO_1X ? soc->pll_video_4x_div : 1U;
		hosc_mult = (want * mult + soc->hosc_hz / 2) / soc->hosc_hz;
		if (hosc_mult == 0)
			hosc_mult = 1;
		return hosc_mult * soc->hosc_hz / mult;
	}
	parent = sunxi_disp_src_rate(tcon->clk, src);
	if (!parent)
		return want;
	div = (parent + want / 2) / want;
	if (div == 0)
		div = 1;
	return parent / div;
}

/* choose module rate and dclk divider closest to the pixel clock */
static int tcon_clk_plan(sunxi_tcon_t *tcon, uint32_t pixclk, uint32_t min_div, uint32_t max_div)
{
	uint32_t best_err = 0xffffffffU, div, rate, pix, err;

	tcon->div = 0;
	if (!pixclk || !min_div || min_div > max_div)
		return DRIVER_ERROR_INVALID;
	for (div = min_div; div <= max_div; div++) {
		if ((uint64_t)pixclk * div > tcon->var->mod_rate_max)
			break;
		rate = mod_round(tcon, pixclk * div);
		pix = (rate + div / 2) / div;
		err = abs_diff(pix, pixclk);
		if (err < best_err) {
			best_err = err;
			tcon->mod_hz = rate;
			tcon->div = div;
			tcon->pixclk_hz = pix;
			if (!err)
				break;
		}
	}
	if (!tcon->div)
		return DRIVER_ERROR_INVALID;
	if (best_err > pixclk / 100)
		pr_warn("pixel clock %u Hz requested, %u Hz possible\n", (unsigned int)pixclk,
			(unsigned int)tcon->pixclk_hz);
	return DRIVER_OK;
}

static int tcon_setup_clock(sunxi_tcon_t *tcon, uint32_t pixclk, uint32_t min_div, uint32_t max_div)
{
	uint32_t got;

	if (tcon_clk_plan(tcon, pixclk, min_div, max_div))
		return DRIVER_ERROR_INVALID;
	got = sunxi_disp_modclk_set(tcon->clk, &tcon->res, &tcon->var->modclk, tcon->mod_hz);
	if (!got) {
		pr_err("cannot set the module clock to %u Hz\n", (unsigned int)tcon->mod_hz);
		return DRIVER_ERROR_INVALID;
	}
	tcon->mod_hz = got;
	tcon->pixclk_hz = (got + tcon->div / 2) / tcon->div;
	tcon_modify_reg(tcon, tcon->var->reg.dclk, TCON0_DCLK_DIV, TPREP(TCON0_DCLK_DIV, tcon->div));
	pr_info("pixel clock %u Hz (module %u Hz / %u)\n", (unsigned int)tcon->pixclk_hz, (unsigned int)tcon->mod_hz,
		(unsigned int)tcon->div);
	return DRIVER_OK;
}

/* DSI: the dot clock is made by the combo phy PLL, the module clock only has to run */
static int tcon_setup_phy_clock(sunxi_tcon_t *tcon, uint32_t pixclk)
{
	/* select the parent, bring up its PLL and open the gate, keep the divider */
	tcon->mod_hz = sunxi_disp_modclk_set(tcon->clk, &tcon->res, &tcon->var->modclk, 0);
	if (!tcon->mod_hz) {
		pr_err("cannot start the module clock\n");
		return DRIVER_ERROR_INVALID;
	}
	tcon->div = 1;
	tcon->pixclk_hz = pixclk;
	tcon_modify_reg(tcon, tcon->var->reg.dclk, TCON0_DCLK_DIV, TPREP(TCON0_DCLK_DIV, 1));
	return DRIVER_OK;
}

/* ------------------------------------------------------------------ */
/* Register programming                                                */
/* ------------------------------------------------------------------ */
static void tcon_init_regs(sunxi_tcon_t *tcon)
{
	/* the pad select is only set for the DSI (trigger) interface */
	tcon_modify_reg(tcon, tcon->var->reg.gctl, TCON_GCTL_IO_MAP_SEL | TCON_GCTL_PAD_SEL,
		tcon->iface == SUNXI_DISP_IF_DSI ? TCON_GCTL_PAD_SEL : 0);
	tcon_modify_reg(tcon, tcon->var->reg.ctl, TCON0_CTL_EN, 0);
	tcon_modify_reg(tcon, tcon->var->reg.gctl, TCON_GCTL_EN, 0);
	tcon_write_reg(tcon, tcon->var->reg.gint0, 0);
	tcon_modify_reg(tcon, tcon->var->reg.gctl, TCON_GCTL_EN, TCON_GCTL_EN);
}

static void tcon_set_timing(sunxi_tcon_t *tcon, const sunxi_disp_timing_t *timing)
{
	uint32_t hbp = timing->hsync_len + timing->hback_porch; /* back porch + sync */
	uint32_t vbp = timing->vsync_len + timing->vback_porch;
	uint32_t vt = tcon->var->vtotal_half_lines ? timing->vtotal * 2U : timing->vtotal;

	tcon_write_reg(tcon, tcon->var->reg.basic0,
		TPREP(TCON0_BASIC0_X, timing->hactive - 1) | TPREP(TCON0_BASIC0_Y, timing->vactive - 1));
	tcon_write_reg(tcon, tcon->var->reg.basic1,
		TPREP(TCON0_BASIC1_HT, timing->htotal - 1) | TPREP(TCON0_BASIC1_HBP, hbp ? hbp - 1 : 0));
	tcon_write_reg(
		tcon, tcon->var->reg.basic2, TPREP(TCON0_BASIC2_VT, vt) | TPREP(TCON0_BASIC2_VBP, vbp ? vbp - 1 : 0));
	tcon_write_reg(tcon, tcon->var->reg.basic3,
		TPREP(TCON0_BASIC3_HSPW, timing->hsync_len ? timing->hsync_len - 1 : 0) |
			TPREP(TCON0_BASIC3_VSPW, timing->vsync_len ? timing->vsync_len - 1 : 0));
}

/* lines between the start of the vertical blank and the data request */
static void tcon_set_start_delay(sunxi_tcon_t *tcon, const sunxi_disp_timing_t *timing)
{
	int32_t delay = (int32_t)timing->vtotal - (int32_t)timing->vactive - (int32_t)tcon->var->start_delay_margin;
	int32_t max = (int32_t)(TCON0_CTL_START_DELAY >> __builtin_ctz(TCON0_CTL_START_DELAY));

	if (delay < (int32_t)tcon->var->start_delay_min)
		delay = tcon->var->start_delay_min;
	if (delay > max)
		delay = max;
	tcon->start_delay = (uint32_t)delay;
	tcon_modify_reg(tcon, tcon->var->reg.ctl, TCON0_CTL_START_DELAY, TPREP(TCON0_CTL_START_DELAY, delay));
}

/* spread the dropped low bits over consecutive frames for 6 bit (or 5/6/5 bit) panels */
static void tcon_set_frm(sunxi_tcon_t *tcon, sunxi_disp_bus_t bus)
{
	static const uint32_t seeds[6] = { 1, 3, 5, 7, 11, 13 };
	static const uint32_t patterns[4] = { 0x01010000, 0x15151111, 0x57575555, 0x7f7f7777 };
	unsigned int i;

	if (bus == SUNXI_DISP_BUS_RGB888) {
		tcon_write_reg(tcon, tcon->var->reg.frm_ctl, 0);
		return;
	}
	for (i = 0; i < 6; i++)
		writel(seeds[i], tcon_reg_address(tcon, tcon->var->reg.frm_seed) + i * 4U);
	for (i = 0; i < 4; i++)
		writel(patterns[i], tcon_reg_address(tcon, tcon->var->reg.frm_tbl) + i * 4U);
	tcon_write_reg(tcon, tcon->var->reg.frm_ctl,
		TCON0_FRM_EN | (bus == SUNXI_DISP_BUS_RGB565 ? TCON0_FRM_MODE_R | TCON0_FRM_MODE_B : 0));
}

static void tcon_set_io(sunxi_tcon_t *tcon, const sunxi_disp_timing_t *timing, uint8_t clk_phase, uint8_t rgb_swap,
	uint8_t rb_swap, uint32_t io_adjust)
{
	static const struct {
		bool inv;
		uint8_t sel;
	} phase[4] = { { false, 0 }, { false, 2 }, { true, 0 }, { true, 2 } };
	uint32_t pol = 0, fifo, fifo_max;

	if (timing->hsync_active)
		pol |= TCON0_IO_HSYNC_POSITIVE;
	if (timing->vsync_active)
		pol |= TCON0_IO_VSYNC_POSITIVE;
	if (!timing->de_active)
		pol |= TCON0_IO_DE_INV;
	if (!clk_phase && !timing->pixelclk_active)
		clk_phase = 2;
	clk_phase &= 3;
	if (phase[clk_phase].inv)
		pol |= TCON0_IO_CLK_INV;
	pol |= TPREP(TCON0_IO_DCLK_SEL, phase[clk_phase].sel);
	tcon_write_reg(tcon, tcon->var->reg.io_pol, pol);

	/* every output driven, no tri-state */
	tcon_write_reg(tcon, tcon->var->reg.io_tri, 0);
	tcon_write_reg(tcon, tcon->var->reg.io_adj, io_adjust);

	tcon_modify_reg(tcon, tcon->var->reg.ctl, TCON0_CTL_RB_SWAP | TCON0_CTL_RGB_SWAP,
		(rb_swap ? TCON0_CTL_RB_SWAP : 0) | TPREP(TCON0_CTL_RGB_SWAP, rgb_swap));

	/* DE data requests are held back for this many FIFO entries */
	fifo_max = TCON_SAFE_PERIOD_FIFO_NUM >> __builtin_ctz(TCON_SAFE_PERIOD_FIFO_NUM);
	fifo = timing->pixel_clock_hz / 1000000U * tcon->var->safe_fifo_per_mhz;
	if (fifo > fifo_max)
		fifo = fifo_max;
	tcon_write_reg(tcon, tcon->var->reg.safe_period,
		TPREP(TCON_SAFE_PERIOD_MODE, tcon->var->safe_period_mode) | TPREP(TCON_SAFE_PERIOD_FIFO_NUM, fifo));
	tcon_write_reg(tcon, tcon->var->reg.fsync_gen_ctrl, 0);
}

static void tcon_config_hv(sunxi_tcon_t *tcon)
{
	const sunxi_tcon_if_cfg_t *if_cfg = &tcon->cfg;

	tcon_modify_reg(tcon, tcon->var->reg.ctl, TCON0_CTL_IF, TPREP(TCON0_CTL_IF, TCON0_IF_HV));
	tcon_write_reg(tcon, tcon->var->reg.hv_ctl,
		TPREP(TCON0_HV_MODE, if_cfg->hv_mode) | TPREP(TCON0_HV_SRGB_SEQ, if_cfg->srgb_seq) |
			TPREP(TCON0_HV_SYUV_SEQ, if_cfg->syuv_seq) | TPREP(TCON0_HV_SYUV_FDLY, if_cfg->syuv_fdly));
	tcon_set_timing(tcon, &tcon->timing);
	tcon_set_start_delay(tcon, &tcon->timing);
	tcon_set_frm(tcon, tcon->bus);
	tcon_set_io(tcon, &tcon->timing, if_cfg->clk_phase, if_cfg->rgb_swap, if_cfg->rb_swap, if_cfg->io_adjust);
}

static void tcon_config_lvds(sunxi_tcon_t *tcon)
{
	uint32_t ctl = TCON0_LVDS_CLK_SEL;

	tcon_modify_reg(tcon, tcon->var->reg.ctl, TCON0_CTL_IF, TPREP(TCON0_CTL_IF, TCON0_IF_HV));
	tcon_write_reg(tcon, tcon->var->reg.hv_ctl, 0);
	if (tcon->cfg.lvds_dual_link)
		ctl |= TCON0_LVDS_LINK;
	if (tcon->cfg.lvds_jeida)
		ctl |= TCON0_LVDS_MODE;
	if (tcon->bus != SUNXI_DISP_BUS_RGB888)
		ctl |= TCON0_LVDS_BITWIDTH;
	tcon_write_reg(tcon, tcon->var->reg.lvds_ctl, ctl); /* enabled in sunxi_tcon_enable */
	tcon_set_timing(tcon, &tcon->timing);
	tcon_set_start_delay(tcon, &tcon->timing);
	tcon_set_frm(tcon, tcon->bus);
	tcon_set_io(tcon, &tcon->timing, 0, 0, 0, 0);
}

/*
 * DSI: the TCON runs in trigger ("CPU") mode and streams lines to the DSI
 * host. In video mode the host generates the frame timing; in command mode
 * the TCON counter paces frames.
 */
static void tcon_config_dsi(sunxi_tcon_t *tcon)
{
	const sunxi_disp_timing_t *timing = &tcon->timing;
	bool cmd = tcon->cfg.dsi_command_mode;
	uint32_t vbp = timing->vsync_len + timing->vback_porch;
	uint32_t delay_lines, start_delay;

	tcon_set_timing(tcon, timing);
	tcon_modify_reg(tcon, tcon->var->reg.ctl, TCON0_CTL_START_DELAY, TPREP(TCON0_CTL_START_DELAY, 0xf));
	tcon_modify_reg(tcon, tcon->var->reg.sync_ctl, TCON_SYNC_DSI_NUM, 0);
	tcon_modify_reg(tcon, tcon->var->reg.ctl, TCON0_CTL_IF, TPREP(TCON0_CTL_IF, cmd ? TCON0_IF_HV : TCON0_IF_CPU));

	tcon_write_reg(tcon, tcon->var->reg.cpu_ctl,
		TPREP(TCON0_CPU_MODE, TCON0_CPU_MODE_DSI) | TCON0_CPU_DA | TCON0_CPU_FLUSH | TCON0_CPU_TRI_FIFO_EN |
			TCON0_CPU_TRI_EN);
	tcon_modify_reg(tcon, tcon->var->reg.ecc_fifo, TCON_ECC_FIFO_SETTING, TPREP(TCON_ECC_FIFO_SETTING, 1U << 3));
	tcon_write_reg(tcon, tcon->var->reg.cpu_tri1, TPREP(TCON0_TRI1_BLOCK_NUM, timing->vactive - 1));

	delay_lines = (vbp > (uint32_t)timing->vsync_len + 3U) ? 3U : vbp;
	if (vbp <= (uint32_t)timing->vsync_len + 3U)
		pr_warn("vertical back porch too short for DSI\n");
	start_delay = delay_lines * timing->htotal / 8U - 1U;

	if (cmd) {
		uint32_t cntr = (uint32_t)timing->htotal * timing->vtotal / 4U, mdiv;

		tcon_write_reg(tcon, tcon->var->reg.cpu_tri0,
			TPREP(TCON0_TRI0_BLOCK_SIZE, timing->hactive - 1) | TPREP(TCON0_TRI0_BLOCK_SPACE, 200 - 1));
		tcon_write_reg(tcon, tcon->var->reg.cpu_tri2,
			TPREP(TCON0_TRI2_START_DELAY, 4 - 1) | TPREP(TCON0_TRI2_TRANS_START_SET, 10 - 1));
		/* no TE: pace frames with the internal counter */
		for (mdiv = 1; mdiv < 256; mdiv++)
			if (cntr / mdiv < 65535)
				break;
		tcon_write_reg(tcon, tcon->var->reg.cpu_tri3,
			TPREP(TCON0_TRI3_COUNTER_M, mdiv - 1) | TPREP(TCON0_TRI3_COUNTER_N, cntr / mdiv - 1) |
				TPREP(TCON0_TRI3_INT_MODE, 1));
	} else {
		tcon_write_reg(tcon, tcon->var->reg.cpu_tri0,
			TPREP(TCON0_TRI0_BLOCK_SIZE, timing->hactive - 1) |
				TPREP(TCON0_TRI0_BLOCK_SPACE, (timing->htotal - timing->hactive) / 2));
		tcon_write_reg(tcon, tcon->var->reg.cpu_tri2,
			TPREP(TCON0_TRI2_START_DELAY, start_delay) | TPREP(TCON0_TRI2_TRANS_START_SET, 10));
	}

	tcon_set_frm(tcon, SUNXI_DISP_BUS_RGB888);
	/* the DSI host owns the data path: the panel's DE / pixel clock polarity does not apply */
	{
		sunxi_disp_timing_t io = *timing;

		io.de_active = 1;
		io.pixelclk_active = 1;
		tcon_set_io(tcon, &io, 0, 0, 0, 0);
	}

	/* the pixel clock comes from the combo phy PLL */
	if (tcon->top && tcon->var->dclk_from_phy_for_dsi) {
		sunxi_tcon_top_lcd_clk_from_phy(tcon->top, tcon->id, tcon->cfg.phy_id, true);
		sunxi_tcon_top_dsi_route(tcon->top, tcon->cfg.dsi_id, tcon->id, true);
	}
}

/* ------------------------------------------------------------------ */
/* Operations                                                          */
/* ------------------------------------------------------------------ */
int sunxi_tcon_prepare(sunxi_tcon_t *tcon, sunxi_disp_if_t iface, const sunxi_disp_timing_t *timing,
	sunxi_disp_bus_t bus, const sunxi_tcon_if_cfg_t *cfg)
{
	int ret;

	if (!tcon || !tcon->var || !timing || !cfg)
		return DRIVER_ERROR_INVALID;
	tcon->iface = iface;
	tcon->timing = *timing;
	tcon->bus = bus;
	tcon->cfg = *cfg;

	if (tcon->top)
		sunxi_tcon_top_get(tcon->top);
	sunxi_disp_res_bus_enable(&tcon->res, true);
	tcon->powered = true;

	tcon_init_regs(tcon);

	switch (iface) {
	case SUNXI_DISP_IF_RGB:
		ret = tcon_setup_clock(tcon, timing->pixel_clock_hz, tcon->var->hv_div_min, tcon->var->hv_div_max);
		if (!ret)
			tcon_config_hv(tcon);
		break;
	case SUNXI_DISP_IF_LVDS:
		/* seven bits per pixel clock on each LVDS pair */
		ret = tcon_setup_clock(tcon, timing->pixel_clock_hz, tcon->var->lvds_div, tcon->var->lvds_div);
		if (!ret)
			tcon_config_lvds(tcon);
		break;
	case SUNXI_DISP_IF_DSI:
		ret = tcon_setup_phy_clock(tcon, timing->pixel_clock_hz);
		if (!ret)
			tcon_config_dsi(tcon);
		break;
	default:
		ret = DRIVER_ERROR_INVALID;
		break;
	}
	if (ret) {
		sunxi_tcon_unprepare(tcon);
		return ret;
	}

	/* take the pixels from the DE port this TCON is linked to */
	tcon_modify_reg(tcon, tcon->var->reg.ctl, TCON0_CTL_SRC_SEL, TPREP(TCON0_CTL_SRC_SEL, TCON0_SRC_DE));
	if (tcon->top)
		sunxi_tcon_top_route_de(tcon->top, tcon->de_port, tcon->id);
	return DRIVER_OK;
}

int sunxi_tcon_enable(sunxi_tcon_t *tcon)
{
	const struct sunxi_tcon_variant *variant = tcon->var;
	int i;

	if (!tcon->powered)
		return DRIVER_ERROR_INVALID;
	if (tcon->top)
		sunxi_tcon_top_lcd_to_pads(tcon->top, tcon->id);

	tcon_write_reg(tcon, tcon->var->reg.gint0, 0); /* flags are cleared by writing 0 */
	tcon_modify_reg(tcon, tcon->var->reg.dclk, TCON0_DCLK_EN, TCON0_DCLK_EN);
	tcon_modify_reg(tcon, tcon->var->reg.ctl, TCON0_CTL_EN, TCON0_CTL_EN);

	if (tcon->iface == SUNXI_DISP_IF_LVDS) {
		tcon_modify_reg(tcon, tcon->var->reg.lvds_ctl, TCON0_LVDS_EN, TCON0_LVDS_EN);
		if (tcon->cfg.lvds_dual_link) {
			uintptr_t ana[2] = { tcon_reg_address(tcon, tcon->var->reg.lvds_ana0),
				tcon_reg_address(tcon, tcon->var->reg.lvds_ana1) };

			/* the second link is driven from the TCON pads */
			for (i = 0; i < 2; i++)
				writel(TPREP(TCON0_LVDS_ANA_C, variant->lvds_ana_c) |
						TPREP(TCON0_LVDS_ANA_R, variant->lvds_ana_r),
					ana[i]);
			udelay(5);
			for (i = 0; i < 2; i++)
				setbits_le32(
					ana[i], TCON0_LVDS_ANA_EN_24M | TCON0_LVDS_ANA_EN_LVDS | TCON0_LVDS_ANA_EN_MB);
			udelay(5);
			for (i = 0; i < 2; i++)
				setbits_le32(ana[i], TCON0_LVDS_ANA_EN_DRVC |
							     TPREP(TCON0_LVDS_ANA_EN_DRVD,
								     tcon->bus != SUNXI_DISP_BUS_RGB888 ? 0x7 : 0xf));
		}
	}
	return DRIVER_OK;
}

void sunxi_tcon_disable(sunxi_tcon_t *tcon)
{
	int i;

	if (!tcon->powered)
		return;
	if (tcon->iface == SUNXI_DISP_IF_LVDS) {
		uintptr_t ana[2] = { tcon_reg_address(tcon, tcon->var->reg.lvds_ana0),
			tcon_reg_address(tcon, tcon->var->reg.lvds_ana1) };

		for (i = 0; i < 2; i++)
			clrbits_le32(ana[i], TCON0_LVDS_ANA_EN_DRVC | TCON0_LVDS_ANA_EN_DRVD);
		udelay(5);
		for (i = 0; i < 2; i++)
			clrbits_le32(ana[i], TCON0_LVDS_ANA_EN_MB | TCON0_LVDS_ANA_SRC_SEL);
		tcon_modify_reg(tcon, tcon->var->reg.lvds_ctl, TCON0_LVDS_EN, 0);
	}
	tcon_modify_reg(tcon, tcon->var->reg.ctl, TCON0_CTL_EN, 0);
	tcon_modify_reg(tcon, tcon->var->reg.dclk, TCON0_DCLK_EN, 0);
	mdelay(20); /* let the panel see a clean end of frame */
}

void sunxi_tcon_unprepare(sunxi_tcon_t *tcon)
{
	if (!tcon->powered)
		return;
	/* the DSI routing stays up until here: panel exit commands need the DSI clocks */
	if (tcon->iface == SUNXI_DISP_IF_DSI && tcon->top && tcon->var->dclk_from_phy_for_dsi) {
		sunxi_tcon_top_dsi_route(tcon->top, tcon->cfg.dsi_id, tcon->id, false);
		sunxi_tcon_top_lcd_clk_from_phy(tcon->top, tcon->id, tcon->cfg.phy_id, false);
	}
	tcon_modify_reg(tcon, tcon->var->reg.gctl, TCON_GCTL_EN, 0);
	tcon_modify_reg(tcon, tcon->var->reg.dclk, TCON0_DCLK_EN, 0);
	tcon_modify_reg(tcon, tcon->var->reg.gctl, TCON_GCTL_PAD_SEL | TCON_GCTL_IO_MAP_SEL, TCON_GCTL_IO_MAP_SEL);
	sunxi_disp_modclk_gate(&tcon->res, false);
	sunxi_disp_res_bus_enable(&tcon->res, false);
	if (tcon->top)
		sunxi_tcon_top_put(tcon->top);
	tcon->powered = false;
}

int sunxi_tcon_set_pattern(sunxi_tcon_t *tcon, sunxi_tcon_pattern_t pattern)
{
	static const uint8_t src[] = {
		[SUNXI_TCON_PATTERN_NONE] = TCON0_SRC_DE,
		[SUNXI_TCON_PATTERN_COLORBAR] = TCON0_SRC_COLORBAR,
		[SUNXI_TCON_PATTERN_GRAYSCALE] = TCON0_SRC_GRAYSCALE,
		[SUNXI_TCON_PATTERN_BLACK_WHITE] = TCON0_SRC_BLACK_WHITE,
		[SUNXI_TCON_PATTERN_BLACK] = TCON0_SRC_BLACK,
		[SUNXI_TCON_PATTERN_WHITE] = TCON0_SRC_WHITE,
		[SUNXI_TCON_PATTERN_GRID] = TCON0_SRC_GRID,
	};

	if ((unsigned int)pattern >= sizeof(src) / sizeof(src[0]) || !tcon->powered)
		return DRIVER_ERROR_INVALID;
	tcon_modify_reg(tcon, tcon->var->reg.ctl, TCON0_CTL_SRC_SEL, TPREP(TCON0_CTL_SRC_SEL, src[pattern]));
	return DRIVER_OK;
}

bool sunxi_tcon_check_underflow(sunxi_tcon_t *tcon)
{
	if (!(tcon_read_reg(tcon, tcon->var->reg.debug) & TCON_DEBUG_UNDERFLOW))
		return false;
	tcon_modify_reg(tcon, tcon->var->reg.debug, TCON_DEBUG_UNDERFLOW, 0);
	return true;
}

uint32_t sunxi_tcon_get_line(sunxi_tcon_t *tcon)
{
	return TGET(TCON_DEBUG_LINE, tcon_read_reg(tcon, tcon->var->reg.debug));
}

void sunxi_tcon_dump(sunxi_tcon_t *tcon)
{
	const struct sunxi_tcon_regs *reg = &tcon->var->reg;
	const uint16_t reg_offsets[] = { reg->gctl, reg->gint0, reg->frm_ctl, reg->ctl, reg->dclk, reg->basic0,
		reg->basic1, reg->basic2, reg->basic3, reg->hv_ctl, reg->cpu_ctl, reg->lvds_ctl, reg->io_pol,
		reg->io_tri, reg->debug, reg->cpu_tri0, reg->cpu_tri1, reg->cpu_tri2, reg->cpu_tri3, reg->safe_period,
		reg->lvds_ana0 };
	unsigned int i;

	pr_debug("%s: pixclk %u Hz (mod %u / %u)\n", tcon->powered ? "on" : "off", (unsigned int)tcon->pixclk_hz,
		(unsigned int)tcon->mod_hz, (unsigned int)tcon->div);
	if (!tcon->powered)
		return;
	for (i = 0; i < sizeof(reg_offsets) / sizeof(reg_offsets[0]); i++)
		pr_debug("  +%03x: %08x\n", reg_offsets[i], (unsigned int)readl(tcon->res.base + reg_offsets[i]));
	if (tcon->top)
		sunxi_tcon_top_dump(tcon->top);
}

/* GINT0 flags are cleared by writing 0; every write rewrites the other flags as 1 */
#define TCON_IRQ_CNTR 10

bool sunxi_tcon_frame_flag(sunxi_tcon_t *tcon)
{
	uint32_t flags;

	if (!tcon->powered)
		return false;
	flags = tcon_read_reg(tcon, tcon->var->reg.gint0);
	if (!(flags & TCON_GINT0_FLAG(TCON_IRQ_CNTR)))
		return false;
	tcon_write_reg(tcon, tcon->var->reg.gint0, (flags | TCON_GINT0_FLAGS) & ~TCON_GINT0_FLAG(TCON_IRQ_CNTR));
	return true;
}

void sunxi_tcon_trigger(sunxi_tcon_t *tcon)
{
	tcon_modify_reg(tcon, tcon->var->reg.cpu_ctl, TCON0_CPU_TRI_START, TCON0_CPU_TRI_START);
}

DT2C_DRIVER_COMPAT("allwinner,sunxi-tcon-lcd");
