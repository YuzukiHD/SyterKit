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

#define REG(t, r)	((t)->res.base + (t)->var->reg.r)
#define RD(t, r)	readl(REG(t, r))
#define WR(t, r, v)	writel((v), REG(t, r))
#define UPD(t, r, m, v) clrsetbits_le32(REG(t, r), (m), (v))

static uint32_t abs_diff(uint32_t a, uint32_t b)
{
	return a > b ? a - b : b - a;
}

/* ------------------------------------------------------------------ */
/* Pixel clock                                                         */
/* ------------------------------------------------------------------ */
static bool src_is_video(sunxi_disp_src_t s)
{
	return s == SUNXI_DISP_SRC_PLL_VIDEO_1X || s == SUNXI_DISP_SRC_PLL_VIDEO_4X;
}

/* the module rate that can really be made for a wanted one */
static uint32_t mod_round(const sunxi_tcon_t *t, uint32_t want)
{
	const struct sunxi_disp_soc *soc = t->clk->soc;
	sunxi_disp_src_t src = t->res.mod_mux < SUNXI_DISP_MAX_PARENTS ? t->var->modclk.parents[t->res.mod_mux] :
									 SUNXI_DISP_SRC_NONE;
	uint32_t mult, n, parent, div;

	if (src_is_video(src)) {
		mult = src == SUNXI_DISP_SRC_PLL_VIDEO_1X ? soc->pll_video_4x_div : 1U;
		n = (want * mult + soc->hosc_hz / 2) / soc->hosc_hz;
		if (n == 0)
			n = 1;
		return n * soc->hosc_hz / mult;
	}
	parent = sunxi_disp_src_rate(t->clk, src);
	if (!parent)
		return want;
	div = (parent + want / 2) / want;
	if (div == 0)
		div = 1;
	return parent / div;
}

/* choose module rate and dclk divider closest to the pixel clock */
static int tcon_clk_plan(sunxi_tcon_t *t, uint32_t pixclk, uint32_t min_div, uint32_t max_div)
{
	uint32_t best_err = 0xffffffffU, div, rate, pix, err;

	t->div = 0;
	if (!pixclk || !min_div || min_div > max_div)
		return DRIVER_ERROR_INVALID;
	for (div = min_div; div <= max_div; div++) {
		if ((uint64_t)pixclk * div > t->var->mod_rate_max)
			break;
		rate = mod_round(t, pixclk * div);
		pix = (rate + div / 2) / div;
		err = abs_diff(pix, pixclk);
		if (err < best_err) {
			best_err = err;
			t->mod_hz = rate;
			t->div = div;
			t->pixclk_hz = pix;
			if (!err)
				break;
		}
	}
	if (!t->div)
		return DRIVER_ERROR_INVALID;
	if (best_err > pixclk / 100)
		pr_warn("pixel clock %u Hz requested, %u Hz possible\n", (unsigned int)pixclk,
			(unsigned int)t->pixclk_hz);
	return DRIVER_OK;
}

static int tcon_setup_clock(sunxi_tcon_t *t, uint32_t pixclk, uint32_t min_div, uint32_t max_div)
{
	uint32_t got;

	if (tcon_clk_plan(t, pixclk, min_div, max_div))
		return DRIVER_ERROR_INVALID;
	got = sunxi_disp_modclk_set(t->clk, &t->res, &t->var->modclk, t->mod_hz);
	if (!got) {
		pr_err("cannot set the module clock to %u Hz\n", (unsigned int)t->mod_hz);
		return DRIVER_ERROR_INVALID;
	}
	t->mod_hz = got;
	t->pixclk_hz = (got + t->div / 2) / t->div;
	UPD(t, dclk, TCON0_DCLK_DIV, TPREP(TCON0_DCLK_DIV, t->div));
	pr_info("pixel clock %u Hz (module %u Hz / %u)\n", (unsigned int)t->pixclk_hz, (unsigned int)t->mod_hz,
		(unsigned int)t->div);
	return DRIVER_OK;
}

/* DSI: the dot clock is made by the combo phy PLL, the module clock only has to run */
static int tcon_setup_phy_clock(sunxi_tcon_t *t, uint32_t pixclk)
{
	/* select the parent, bring up its PLL and open the gate, keep the divider */
	t->mod_hz = sunxi_disp_modclk_set(t->clk, &t->res, &t->var->modclk, 0);
	if (!t->mod_hz) {
		pr_err("cannot start the module clock\n");
		return DRIVER_ERROR_INVALID;
	}
	t->div = 1;
	t->pixclk_hz = pixclk;
	UPD(t, dclk, TCON0_DCLK_DIV, TPREP(TCON0_DCLK_DIV, 1));
	return DRIVER_OK;
}

/* ------------------------------------------------------------------ */
/* Register programming                                                */
/* ------------------------------------------------------------------ */
static void tcon_init_regs(sunxi_tcon_t *t)
{
	UPD(t, gctl, TCON_GCTL_IO_MAP_SEL | TCON_GCTL_PAD_SEL, TCON_GCTL_PAD_SEL);
	UPD(t, ctl, TCON0_CTL_EN, 0);
	UPD(t, gctl, TCON_GCTL_EN, 0);
	WR(t, gint0, 0);
	UPD(t, gctl, TCON_GCTL_EN, TCON_GCTL_EN);
}

static void tcon_set_timing(sunxi_tcon_t *t, const sunxi_disp_timing_t *m)
{
	uint32_t hbp = m->hsync_len + m->hback_porch; /* back porch + sync */
	uint32_t vbp = m->vsync_len + m->vback_porch;
	uint32_t vt = t->var->vtotal_half_lines ? m->vtotal * 2U : m->vtotal;

	WR(t, basic0, TPREP(TCON0_BASIC0_X, m->hactive - 1) | TPREP(TCON0_BASIC0_Y, m->vactive - 1));
	WR(t, basic1, TPREP(TCON0_BASIC1_HT, m->htotal - 1) | TPREP(TCON0_BASIC1_HBP, hbp ? hbp - 1 : 0));
	WR(t, basic2, TPREP(TCON0_BASIC2_VT, vt) | TPREP(TCON0_BASIC2_VBP, vbp ? vbp - 1 : 0));
	WR(t, basic3,
		TPREP(TCON0_BASIC3_HSPW, m->hsync_len ? m->hsync_len - 1 : 0) |
			TPREP(TCON0_BASIC3_VSPW, m->vsync_len ? m->vsync_len - 1 : 0));
}

/* lines between the start of the vertical blank and the data request */
static void tcon_set_start_delay(sunxi_tcon_t *t, const sunxi_disp_timing_t *m)
{
	int32_t delay = (int32_t)m->vtotal - (int32_t)m->vactive - (int32_t)t->var->start_delay_margin;
	int32_t max = (int32_t)(TCON0_CTL_START_DELAY >> __builtin_ctz(TCON0_CTL_START_DELAY));

	if (delay < (int32_t)t->var->start_delay_min)
		delay = t->var->start_delay_min;
	if (delay > max)
		delay = max;
	t->start_delay = (uint32_t)delay;
	UPD(t, ctl, TCON0_CTL_START_DELAY, TPREP(TCON0_CTL_START_DELAY, delay));
}

/* spread the dropped low bits over consecutive frames for 6 bit (or 5/6/5 bit) panels */
static void tcon_set_frm(sunxi_tcon_t *t, sunxi_disp_bus_t bus)
{
	static const uint32_t seeds[6] = { 1, 3, 5, 7, 11, 13 };
	static const uint32_t patterns[4] = { 0x01010000, 0x15151111, 0x57575555, 0x7f7f7777 };
	unsigned int i;

	if (bus == SUNXI_DISP_BUS_RGB888) {
		WR(t, frm_ctl, 0);
		return;
	}
	for (i = 0; i < 6; i++)
		writel(seeds[i], REG(t, frm_seed) + i * 4U);
	for (i = 0; i < 4; i++)
		writel(patterns[i], REG(t, frm_tbl) + i * 4U);
	WR(t, frm_ctl, TCON0_FRM_EN | (bus == SUNXI_DISP_BUS_RGB565 ? TCON0_FRM_MODE_R | TCON0_FRM_MODE_B : 0));
}

static void tcon_set_io(sunxi_tcon_t *t, const sunxi_disp_timing_t *m, uint8_t clk_phase, uint8_t rgb_swap,
	uint8_t rb_swap, uint32_t io_adjust)
{
	static const struct {
		bool inv;
		uint8_t sel;
	} phase[4] = { { false, 0 }, { false, 2 }, { true, 0 }, { true, 2 } };
	uint32_t pol = 0, fifo, fifo_max;

	if (m->hsync_active)
		pol |= TCON0_IO_HSYNC_POSITIVE;
	if (m->vsync_active)
		pol |= TCON0_IO_VSYNC_POSITIVE;
	if (!m->de_active)
		pol |= TCON0_IO_DE_INV;
	if (!clk_phase && !m->pixelclk_active)
		clk_phase = 2;
	clk_phase &= 3;
	if (phase[clk_phase].inv)
		pol |= TCON0_IO_CLK_INV;
	pol |= TPREP(TCON0_IO_DCLK_SEL, phase[clk_phase].sel);
	WR(t, io_pol, pol);

	/* every output driven, no tri-state */
	WR(t, io_tri, 0);
	WR(t, io_adj, io_adjust);

	UPD(t, ctl, TCON0_CTL_RB_SWAP | TCON0_CTL_RGB_SWAP,
		(rb_swap ? TCON0_CTL_RB_SWAP : 0) | TPREP(TCON0_CTL_RGB_SWAP, rgb_swap));

	/* DE data requests are held back for this many FIFO entries */
	fifo_max = TCON_SAFE_PERIOD_FIFO_NUM >> __builtin_ctz(TCON_SAFE_PERIOD_FIFO_NUM);
	fifo = m->pixel_clock_hz / 1000000U * t->var->safe_fifo_per_mhz;
	if (fifo > fifo_max)
		fifo = fifo_max;
	WR(t, safe_period,
		TPREP(TCON_SAFE_PERIOD_MODE, t->var->safe_period_mode) | TPREP(TCON_SAFE_PERIOD_FIFO_NUM, fifo));
	WR(t, fsync_gen_ctrl, 0);
}

static void tcon_config_hv(sunxi_tcon_t *t)
{
	const sunxi_tcon_if_cfg_t *c = &t->cfg;

	UPD(t, ctl, TCON0_CTL_IF, TPREP(TCON0_CTL_IF, TCON0_IF_HV));
	WR(t, hv_ctl,
		TPREP(TCON0_HV_MODE, c->hv_mode) | TPREP(TCON0_HV_SRGB_SEQ, c->srgb_seq) |
			TPREP(TCON0_HV_SYUV_SEQ, c->syuv_seq) | TPREP(TCON0_HV_SYUV_FDLY, c->syuv_fdly));
	tcon_set_timing(t, &t->timing);
	tcon_set_start_delay(t, &t->timing);
	tcon_set_frm(t, t->bus);
	tcon_set_io(t, &t->timing, c->clk_phase, c->rgb_swap, c->rb_swap, c->io_adjust);
}

static void tcon_config_lvds(sunxi_tcon_t *t)
{
	uint32_t ctl = TCON0_LVDS_CLK_SEL;

	UPD(t, ctl, TCON0_CTL_IF, TPREP(TCON0_CTL_IF, TCON0_IF_HV));
	WR(t, hv_ctl, 0);
	if (t->cfg.lvds_dual_link)
		ctl |= TCON0_LVDS_LINK;
	if (t->cfg.lvds_jeida)
		ctl |= TCON0_LVDS_MODE;
	if (t->bus != SUNXI_DISP_BUS_RGB888)
		ctl |= TCON0_LVDS_BITWIDTH;
	WR(t, lvds_ctl, ctl); /* enabled in sunxi_tcon_enable */
	tcon_set_timing(t, &t->timing);
	tcon_set_start_delay(t, &t->timing);
	tcon_set_frm(t, t->bus);
	tcon_set_io(t, &t->timing, 0, 0, 0, 0);
}

/*
 * DSI: the TCON runs in trigger ("CPU") mode and streams lines to the DSI
 * host. In video mode the host generates the frame timing; in command mode
 * the TCON counter paces frames.
 */
static void tcon_config_dsi(sunxi_tcon_t *t)
{
	const sunxi_disp_timing_t *m = &t->timing;
	bool cmd = t->cfg.dsi_command_mode;
	uint32_t vbp = m->vsync_len + m->vback_porch;
	uint32_t delay_lines, start_delay;

	tcon_set_timing(t, m);
	UPD(t, ctl, TCON0_CTL_START_DELAY, TPREP(TCON0_CTL_START_DELAY, 0xf));
	UPD(t, sync_ctl, TCON_SYNC_DSI_NUM, 0);
	UPD(t, ctl, TCON0_CTL_IF, TPREP(TCON0_CTL_IF, cmd ? TCON0_IF_HV : TCON0_IF_CPU));

	WR(t, cpu_ctl,
		TPREP(TCON0_CPU_MODE, TCON0_CPU_MODE_DSI) | TCON0_CPU_DA | TCON0_CPU_FLUSH | TCON0_CPU_TRI_FIFO_EN |
			TCON0_CPU_TRI_EN);
	UPD(t, ecc_fifo, TCON_ECC_FIFO_SETTING, TPREP(TCON_ECC_FIFO_SETTING, 1U << 3));
	WR(t, cpu_tri1, TPREP(TCON0_TRI1_BLOCK_NUM, m->vactive - 1));

	delay_lines = (vbp > (uint32_t)m->vsync_len + 3U) ? 3U : vbp;
	if (vbp <= (uint32_t)m->vsync_len + 3U)
		pr_warn("vertical back porch too short for DSI\n");
	start_delay = delay_lines * m->htotal / 8U - 1U;

	if (cmd) {
		uint32_t cntr = (uint32_t)m->htotal * m->vtotal / 4U, mdiv;

		WR(t, cpu_tri0, TPREP(TCON0_TRI0_BLOCK_SIZE, m->hactive - 1) | TPREP(TCON0_TRI0_BLOCK_SPACE, 200 - 1));
		WR(t, cpu_tri2, TPREP(TCON0_TRI2_START_DELAY, 4 - 1) | TPREP(TCON0_TRI2_TRANS_START_SET, 10 - 1));
		/* no TE: pace frames with the internal counter */
		for (mdiv = 1; mdiv < 256; mdiv++)
			if (cntr / mdiv < 65535)
				break;
		WR(t, cpu_tri3,
			TPREP(TCON0_TRI3_COUNTER_M, mdiv - 1) | TPREP(TCON0_TRI3_COUNTER_N, cntr / mdiv - 1) |
				TPREP(TCON0_TRI3_INT_MODE, 1));
	} else {
		WR(t, cpu_tri0,
			TPREP(TCON0_TRI0_BLOCK_SIZE, m->hactive - 1) |
				TPREP(TCON0_TRI0_BLOCK_SPACE, (m->htotal - m->hactive) / 2));
		WR(t, cpu_tri2, TPREP(TCON0_TRI2_START_DELAY, start_delay) | TPREP(TCON0_TRI2_TRANS_START_SET, 10));
	}

	tcon_set_frm(t, SUNXI_DISP_BUS_RGB888);
	/* the DSI host owns the data path: the panel's DE / pixel clock polarity does not apply */
	{
		sunxi_disp_timing_t io = *m;

		io.de_active = 1;
		io.pixelclk_active = 1;
		tcon_set_io(t, &io, 0, 0, 0, 0);
	}

	/* the pixel clock comes from the combo phy PLL */
	if (t->top && t->var->dclk_from_phy_for_dsi) {
		sunxi_tcon_top_lcd_clk_from_phy(t->top, t->id, t->cfg.phy_id, true);
		sunxi_tcon_top_dsi_route(t->top, t->cfg.dsi_id, t->id, true);
	}
}

/* ------------------------------------------------------------------ */
/* Operations                                                          */
/* ------------------------------------------------------------------ */
int sunxi_tcon_prepare(sunxi_tcon_t *t, sunxi_disp_if_t iface, const sunxi_disp_timing_t *timing, sunxi_disp_bus_t bus,
	const sunxi_tcon_if_cfg_t *cfg)
{
	int ret;

	if (!t || !t->var || !timing || !cfg)
		return DRIVER_ERROR_INVALID;
	t->iface = iface;
	t->timing = *timing;
	t->bus = bus;
	t->cfg = *cfg;

	if (t->top)
		sunxi_tcon_top_get(t->top);
	sunxi_disp_res_bus_enable(&t->res, true);
	t->powered = true;

	tcon_init_regs(t);

	switch (iface) {
	case SUNXI_DISP_IF_RGB:
		ret = tcon_setup_clock(t, timing->pixel_clock_hz, t->var->hv_div_min, t->var->hv_div_max);
		if (!ret)
			tcon_config_hv(t);
		break;
	case SUNXI_DISP_IF_LVDS:
		/* seven bits per pixel clock on each LVDS pair */
		ret = tcon_setup_clock(t, timing->pixel_clock_hz, t->var->lvds_div, t->var->lvds_div);
		if (!ret)
			tcon_config_lvds(t);
		break;
	case SUNXI_DISP_IF_DSI:
		ret = tcon_setup_phy_clock(t, timing->pixel_clock_hz);
		if (!ret)
			tcon_config_dsi(t);
		break;
	default:
		ret = DRIVER_ERROR_INVALID;
		break;
	}
	if (ret) {
		sunxi_tcon_unprepare(t);
		return ret;
	}

	/* take the pixels from the DE port this TCON is linked to */
	UPD(t, ctl, TCON0_CTL_SRC_SEL, TPREP(TCON0_CTL_SRC_SEL, TCON0_SRC_DE));
	if (t->top)
		sunxi_tcon_top_route_de(t->top, t->de_port, t->id);
	return DRIVER_OK;
}

int sunxi_tcon_enable(sunxi_tcon_t *t)
{
	const struct sunxi_tcon_variant *v = t->var;
	int i;

	if (!t->powered)
		return DRIVER_ERROR_INVALID;
	if (t->top)
		sunxi_tcon_top_lcd_to_pads(t->top, t->id);

	WR(t, gint0, 0); /* flags are cleared by writing 0 */
	UPD(t, dclk, TCON0_DCLK_EN, TCON0_DCLK_EN);
	UPD(t, ctl, TCON0_CTL_EN, TCON0_CTL_EN);

	if (t->iface == SUNXI_DISP_IF_LVDS) {
		UPD(t, lvds_ctl, TCON0_LVDS_EN, TCON0_LVDS_EN);
		if (t->cfg.lvds_dual_link) {
			uintptr_t ana[2] = { REG(t, lvds_ana0), REG(t, lvds_ana1) };

			/* the second link is driven from the TCON pads */
			for (i = 0; i < 2; i++)
				writel(TPREP(TCON0_LVDS_ANA_C, v->lvds_ana_c) | TPREP(TCON0_LVDS_ANA_R, v->lvds_ana_r),
					ana[i]);
			udelay(5);
			for (i = 0; i < 2; i++)
				setbits_le32(
					ana[i], TCON0_LVDS_ANA_EN_24M | TCON0_LVDS_ANA_EN_LVDS | TCON0_LVDS_ANA_EN_MB);
			udelay(5);
			for (i = 0; i < 2; i++)
				setbits_le32(ana[i],
					TCON0_LVDS_ANA_EN_DRVC | TPREP(TCON0_LVDS_ANA_EN_DRVD,
									 t->bus != SUNXI_DISP_BUS_RGB888 ? 0x7 : 0xf));
		}
	}
	return DRIVER_OK;
}

void sunxi_tcon_disable(sunxi_tcon_t *t)
{
	int i;

	if (!t->powered)
		return;
	if (t->iface == SUNXI_DISP_IF_LVDS) {
		uintptr_t ana[2] = { REG(t, lvds_ana0), REG(t, lvds_ana1) };

		for (i = 0; i < 2; i++)
			clrbits_le32(ana[i], TCON0_LVDS_ANA_EN_DRVC | TCON0_LVDS_ANA_EN_DRVD);
		udelay(5);
		for (i = 0; i < 2; i++)
			clrbits_le32(ana[i], TCON0_LVDS_ANA_EN_MB | TCON0_LVDS_ANA_SRC_SEL);
		UPD(t, lvds_ctl, TCON0_LVDS_EN, 0);
	}
	UPD(t, ctl, TCON0_CTL_EN, 0);
	UPD(t, dclk, TCON0_DCLK_EN, 0);
	mdelay(20); /* let the panel see a clean end of frame */
}

void sunxi_tcon_unprepare(sunxi_tcon_t *t)
{
	if (!t->powered)
		return;
	/* the DSI routing stays up until here: panel exit commands need the DSI clocks */
	if (t->iface == SUNXI_DISP_IF_DSI && t->top && t->var->dclk_from_phy_for_dsi) {
		sunxi_tcon_top_dsi_route(t->top, t->cfg.dsi_id, t->id, false);
		sunxi_tcon_top_lcd_clk_from_phy(t->top, t->id, t->cfg.phy_id, false);
	}
	UPD(t, gctl, TCON_GCTL_EN, 0);
	UPD(t, dclk, TCON0_DCLK_EN, 0);
	UPD(t, gctl, TCON_GCTL_PAD_SEL | TCON_GCTL_IO_MAP_SEL, TCON_GCTL_IO_MAP_SEL);
	sunxi_disp_modclk_gate(&t->res, false);
	sunxi_disp_res_bus_enable(&t->res, false);
	if (t->top)
		sunxi_tcon_top_put(t->top);
	t->powered = false;
}

int sunxi_tcon_set_pattern(sunxi_tcon_t *t, sunxi_tcon_pattern_t pattern)
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

	if ((unsigned int)pattern >= sizeof(src) / sizeof(src[0]) || !t->powered)
		return DRIVER_ERROR_INVALID;
	UPD(t, ctl, TCON0_CTL_SRC_SEL, TPREP(TCON0_CTL_SRC_SEL, src[pattern]));
	return DRIVER_OK;
}

bool sunxi_tcon_check_underflow(sunxi_tcon_t *t)
{
	if (!(RD(t, debug) & TCON_DEBUG_UNDERFLOW))
		return false;
	UPD(t, debug, TCON_DEBUG_UNDERFLOW, 0);
	return true;
}

uint32_t sunxi_tcon_get_line(sunxi_tcon_t *t)
{
	return TGET(TCON_DEBUG_LINE, RD(t, debug));
}

void sunxi_tcon_dump(sunxi_tcon_t *t)
{
	const struct sunxi_tcon_regs *r = &t->var->reg;
	const uint16_t regs[] = { r->gctl, r->gint0, r->frm_ctl, r->ctl, r->dclk, r->basic0, r->basic1, r->basic2,
		r->basic3, r->hv_ctl, r->cpu_ctl, r->lvds_ctl, r->io_pol, r->io_tri, r->debug, r->cpu_tri0, r->cpu_tri2,
		r->safe_period, r->lvds_ana0 };
	unsigned int i;

	pr_debug("%s: pixclk %u Hz (mod %u / %u)\n", t->powered ? "on" : "off", (unsigned int)t->pixclk_hz,
		(unsigned int)t->mod_hz, (unsigned int)t->div);
	if (!t->powered)
		return;
	for (i = 0; i < sizeof(regs) / sizeof(regs[0]); i++)
		pr_debug("  +%03x: %08x\n", regs[i], (unsigned int)readl(t->res.base + regs[i]));
	if (t->top)
		sunxi_tcon_top_dump(t->top);
}

DT2C_DRIVER_COMPAT("allwinner,sunxi-tcon-lcd");
