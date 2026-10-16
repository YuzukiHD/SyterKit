// SPDX-License-Identifier: GPL-2.0+
/*
 * sun252iw1 display engine (DE 2.0 class, "v203"): one mixer, no scaler. The vendor driver runs it in AHB
 * mode (no RCQ, no double buffer register): registers are written straight through, like sun252iw2.
 *
 *   top:        0x000 module clock gate, 0x004 bus clock gate, 0x008 AHB reset (bit 0 = mixer 0)
 *   mixer 0:    GLB 0x100000, blender 0x101000
 *   channels:   0x102000 VI (2 layers), 0x103000 UI (2 layers)
 *
 * Only UI channel 1 is used, it reaches the blender through route value 1.
 */
#include <drivers/display/sun252iw1.h>

#include "../../de/de-regs.h"

#define DE_GLB	0x100000
#define DE_BLD	0x101000
#define DE_UI_CH 0x103000

const struct sunxi_de_variant sunxi_de_variant_sun252iw1 = {
	.name = "sun252iw1",
	/* mux 0: PLL_PERI(300M), mux 1: PLL_VIDEO(1X), 5 bit linear divider */
	.modclk = {
		.div = SUNXI_DISP_DIV_LINEAR,
		.div_width = 5,
		.parents = { SUN252IW1_DISP_SRC_PLL_PERI_300M, SUN252IW1_DISP_SRC_PLL_VIDEO_1X },
	},
	.default_mod_hz = 300000000,
	.port = 0,
	.commit = SUNXI_DE_COMMIT_DIRECT,

	.top_reset = 0x008,
	.top_ahb_gate = 0x004,
	.top_no_mbus_gate = 1,
	.top_clk_gate = 0x000,
	.core_shift = 0,
	.glb_ctl = DE_GLB + 0x00,
	.glb_size = DE_GLB + 0x0c,
	.glb_ctl_enable = 1U << 0,
	.size_w_shift = 0,
	.size_h_shift = 16,
	.size_w_mask = 0x1fff,
	.size_h_mask = 0x1fff,
	.size_minus_one = 1,

	.ovl_base = DE_UI_CH,
	.layer_attr = 0x00,
	.layer_size = 0x04,
	.layer_coor = 0x08,
	.layer_pitch = 0x0c,
	.layer_laddr = 0x10,
	.layer_bot_laddr = 0x14,
	.layer_fcolor = 0x18,
	.misc_off = 0x80,
	.misc_haddr = 0x00,
	.misc_bot_haddr = 0x04,
	.misc_ovl_size = 0x08,
	.layer_stride = 0x20,
	.nlayers = 2,
	.attr_en = 1U << 0,
	.attr_alpha_mode_shift = 1,
	.attr_fmt_shift = 8,
	.attr_alpha_shift = 24,
	.alpha_mode_pixel = 0,
	.alpha_mode_layer = 1,
	.fmt = {
		[SUNXI_DE_FMT_ARGB8888] = 0x00,
		[SUNXI_DE_FMT_XRGB8888] = 0x04,
		[SUNXI_DE_FMT_RGB888] = 0x08,
		[SUNXI_DE_FMT_RGB565] = 0x0a,
	},
	.fmt_cpp = {
		[SUNXI_DE_FMT_ARGB8888] = 4,
		[SUNXI_DE_FMT_XRGB8888] = 4,
		[SUNXI_DE_FMT_RGB888] = 3,
		[SUNXI_DE_FMT_RGB565] = 2,
	},

	.bld_base = DE_BLD,
	.bld_fill_ctl = 0x000,
	.bld_pipe_fill_bit = 0,
	.bld_pipe_en_bit = 8,
	.bld_pipe_fcolor = 0x004,
	.bld_pipe_size = 0x008,
	.bld_pipe_offset = 0x00c,
	.bld_pipe_stride = 0x10,
	.bld_ctl_block = 0x080,
	.bld_route = 0x000,
	.bld_premul = 0x004,
	.bld_bkcolor = 0x008,
	.bld_out_size = 0x00c,
	.bld_mode = 0x010,
	.bld_route_width = 4,
	.bld_npipes = 2,
	/* source over: Cs * 1 + Cd * (1 - As) for colour and alpha */
	.bld_src_over = 1U | (3U << 8) | (1U << 16) | (3U << 24),
	.bld_ck_block = 0x0b0,
	.bld_ck_ctl = 0x000,
	.bld_out_ctl = 0x04c,
	.chn_port = 1,

	/* gamma, CSC and dither are off after reset and the BSP does not touch them for plain RGB output */
	.nbypass = 0,
};
