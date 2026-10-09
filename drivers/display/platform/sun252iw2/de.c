// SPDX-License-Identifier: GPL-2.0+
/*
 * sun252iw2 display engine (DE 2.1 class): one mixer, UI channel 0 in slot 3.
 *
 *   channel slots at 0x100000 + slot * 0x20000, UI channel 0 = slot 3
 *   mixer 0 at 0x1c0000: display CSC 0x100, blender 0x1000, dither 0x8000, gamma 0x9000
 */
#include "../../de/de-regs.h"

#define DE_CHN(slot) (0x100000 + (slot) * 0x20000)
#define DE_DISP0     0x1c0000

const struct sunxi_de_variant sunxi_de_variant_sun252iw2 = {
	.name = "sun252iw2",
	/* mux 0: PLL_PERI(2X), mux 1: PLL_VIDEO(4X), 5 bit linear divider */
	.modclk = {
		.div = SUNXI_DISP_DIV_LINEAR,
		.div_width = 5,
		.parents = { SUNXI_DISP_SRC_PLL_PERI_2X, SUNXI_DISP_SRC_PLL_VIDEO_4X },
	},
	.default_mod_hz = 300000000,
	.port = 0,
	.commit = SUNXI_DE_COMMIT_DIRECT,

	.top_reset = 0x010,
	.top_ahb_gate = 0x014,
	.top_mbus_gate = 0x018,
	.top_clk_gate = 0x01c,
	.core_shift = 0,
	.glb_ctl = 0x100,
	.glb_size = 0x108,
	.glb_ctl_enable = 1U << 0,
	.size_w_shift = 0,
	.size_h_shift = 16,
	.size_w_mask = 0x1fff,
	.size_h_mask = 0x1fff,
	.size_minus_one = 1,

	.ovl_base = DE_CHN(3) + 0x1000,
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
	.nlayers = 4,
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

	.bld_base = DE_DISP0 + 0x1000,
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
	.bld_npipes = 4,
	/* source over: Cs * 1 + Cd * (1 - As) for colour and alpha */
	.bld_src_over = 1U | (3U << 8) | (1U << 16) | (3U << 24),
	.bld_ck_block = 0x0b0,
	.bld_ck_ctl = 0x000,
	.bld_out_ctl = 0x04c,
	.chn_port = 1,

	/* behind the blender: display CSC off, gamma colour matrix / table / CTC off, dither off */
	.bypass = {
		{ DE_DISP0 + 0x100, 0 },
		{ DE_DISP0 + 0x9000, 0 },
		{ DE_DISP0 + 0x9040, 0 },
		{ DE_DISP0 + 0x9050, 0 },
		{ DE_DISP0 + 0x8000, 0 },
	},
	.nbypass = 5,
};
