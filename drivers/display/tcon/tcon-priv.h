/* SPDX-License-Identifier: GPL-2.0+ */
#ifndef __TCON_PRIV_H__
#define __TCON_PRIV_H__

#include <drivers/display/display.h>

/* Register offsets of one TCON LCD generation. */
struct sunxi_tcon_regs {
	uint16_t gctl, gint0, gint1;
	uint16_t frm_ctl, frm_seed, frm_tbl; /* seed/tbl: first of an array of u32 */
	uint16_t ctl, dclk, basic0, basic1, basic2, basic3;
	uint16_t hv_ctl, cpu_ctl, lvds_ctl, io_pol, io_tri, io_adj;
	uint16_t ecc_fifo, debug;
	uint16_t cpu_tri0, cpu_tri1, cpu_tri2, cpu_tri3, cpu_tri4;
	uint16_t safe_period, lvds_ana0, lvds_ana1, sync_ctl, fsync_gen_ctrl;
};

struct sunxi_tcon_variant {
	const char *name;
	struct sunxi_tcon_regs reg;
	sunxi_disp_modclk_t modclk;
	uint32_t mod_rate_max; /* highest module clock, Hz */
	uint8_t hv_div_min, hv_div_max; /* pixel clock divider range */
	uint8_t lvds_div; /* fixed divider: 7 bits per LVDS pair */
	uint8_t start_delay_min, start_delay_margin;
	uint8_t safe_period_mode;
	uint8_t safe_fifo_per_mhz;
	uint8_t lvds_ana_c, lvds_ana_r;
	uint8_t vtotal_half_lines; /* BASIC2 vertical total counts half lines */
	uint8_t dclk_from_phy_for_dsi; /* DSI pixel clock comes from the combo phy via the top */
};

struct sunxi_tcon_top_regs {
	uint16_t tv_setup, dsi_src, clk_src, de_perh, clk_gate;
};

struct sunxi_tcon_top_variant {
	const char *name;
	struct sunxi_tcon_top_regs reg;
	uint8_t n_de_ports, n_tcons, n_dsi_clk_gates;
};

const struct sunxi_tcon_variant *sunxi_tcon_find_variant(const char *name);
const struct sunxi_tcon_top_variant *sunxi_tcon_top_find_variant(const char *name);

int sunxi_tcon_top_get(sunxi_tcon_top_t *top);
void sunxi_tcon_top_put(sunxi_tcon_top_t *top);
void sunxi_tcon_top_route_de(sunxi_tcon_top_t *top, uint32_t de_port, uint32_t tcon_id);
void sunxi_tcon_top_lcd_to_pads(sunxi_tcon_top_t *top, uint32_t tcon_id);
void sunxi_tcon_top_lcd_clk_from_phy(sunxi_tcon_top_t *top, uint32_t tcon_id, uint32_t phy_id, bool from_phy);
void sunxi_tcon_top_dsi_route(sunxi_tcon_top_t *top, uint32_t dsi_id, uint32_t tcon_id, bool enable);
void sunxi_tcon_top_dump(sunxi_tcon_top_t *top);

#endif
