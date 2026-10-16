/* SPDX-License-Identifier: GPL-2.0+ */
#include <drivers/display/sun252iw1.h>

#include "../../tcon/tcon-priv.h"

/*
 * sun252iw1: one TCON LCD (parallel / serial RGB and i8080, no LVDS or DSI pads), TCON top at 0x05460000.
 * The register layout equals sun252iw2 up to 0x1f0, only FSYNC moved. The DSI / LVDS offsets are kept
 * for the shared code paths, the chip never reaches them.
 */
const struct sunxi_tcon_variant sunxi_tcon_variant_sun252iw1 = {
	.name = "sun252iw1",
	.reg = {
		.gctl = 0x000, .gint0 = 0x004, .gint1 = 0x008,
		.frm_ctl = 0x010, .frm_seed = 0x014, .frm_tbl = 0x02c,
		.ctl = 0x040, .dclk = 0x044,
		.basic0 = 0x048, .basic1 = 0x04c, .basic2 = 0x050, .basic3 = 0x054,
		.hv_ctl = 0x058, .cpu_ctl = 0x060, .cpu_wr = 0x064, .lvds_ctl = 0x084,
		.io_pol = 0x088, .io_tri = 0x08c, .io_adj = 0x090,
		.ecc_fifo = 0x0f8, .debug = 0x0fc,
		.cpu_tri0 = 0x160, .cpu_tri1 = 0x164, .cpu_tri2 = 0x168,
		.cpu_tri3 = 0x16c, .cpu_tri4 = 0x170,
		.safe_period = 0x1f0, .lvds_ana0 = 0x220, .lvds_ana1 = 0x224,
		.sync_ctl = 0x230, .fsync_gen_ctrl = 0x228,
	},
	/*
	 * gate 31, mux 26:24 = { PLL_VIDEO(4X), PLL_PERI(2X), PLL_CSI(4X) }, P 9:8, M 3:0.
	 * PLL_VIDEO can not be used for slow pixel clocks: its minimum is 1272 MHz and the shared clock code
	 * retunes it to the module rate, so the nodes select PLL_PERI(2X) and divide with M.
	 */
	.modclk = {
		.div = SUNXI_DISP_DIV_MP,
		.parents = { SUN252IW1_DISP_SRC_PLL_VIDEO_4X, SUN252IW1_DISP_SRC_PLL_PERI_2X, SUNXI_DISP_SRC_NONE,
			     SUNXI_DISP_SRC_NONE },
	},
	.mod_rate_max = 600000000U,
	.hv_div_min = 6, .hv_div_max = 127,
	.lvds_div = 7,
	.start_delay_min = 10, .start_delay_margin = 8,
	.safe_period_mode = 3, .safe_fifo_per_mhz = 15,
	.lvds_ana_c = 4, .lvds_ana_r = 3,
	.vtotal_half_lines = 1,
};

const struct sunxi_tcon_top_variant sunxi_tcon_top_variant_sun252iw1 = {
	.name = "sun252iw1",
	.reg = { .tv_setup = 0x000, .dsi_src = 0x004, .clk_src = 0x00c, .de_perh = 0x01c, .clk_gate = 0x020 },
	.n_de_ports = 1,
	.n_tcons = 1,
	.n_dsi_clk_gates = 0,
	/* rgb0_src_sel (17:16) = 0: the LCD pads belong to TCON LCD0 */
	.lcd_pad_clr = { 3U << 16, 0 },
};
