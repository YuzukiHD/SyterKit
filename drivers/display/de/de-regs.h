/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Display engine description for the UI passthrough: one UI channel
 * (layer 0, no scaler) into one blender pipe, everything behind the blender
 * (colour space converter, gamma, dither) switched off.
 *
 * All offsets, field positions and format codes that differ between engine
 * generations are in struct sunxi_de_variant; de-ui.c has no chip literals.
 */
#ifndef __SUNXI_DE_REGS_H__
#define __SUNXI_DE_REGS_H__

#include <stdint.h>

#include <drivers/display/display.h>

/* how the registers of the channel / blender are brought to the hardware */
typedef enum {
	SUNXI_DE_COMMIT_DIRECT = 0,	/* plain register writes (output may tear for a frame) */
} sunxi_de_commit_t;

/* fixed register write that puts a stage (offset from the engine base) into bypass */
struct sunxi_de_bypass {
	uint32_t offset;
	uint32_t value;
};

#define SUNXI_DE_MAX_BYPASS 8

struct sunxi_de_variant {
	const char *name;			/* value of allwinner,variant */
	sunxi_disp_modclk_t modclk;		/* module clock of the engine */
	uint32_t default_mod_hz;
	uint8_t port;				/* output port towards the TCON top */
	sunxi_de_commit_t commit;

	/* engine top: per mixer reset / gate registers, one bit (core_shift) each */
	uint16_t top_reset, top_ahb_gate, top_mbus_gate, top_clk_gate;
	uint8_t core_shift;
	/* global control of the mixer */
	uint16_t glb_ctl, glb_size;
	uint32_t glb_ctl_enable;
	uint8_t size_w_shift, size_h_shift;
	uint32_t size_w_mask, size_h_mask;	/* masks of the value, unshifted */

	/* UI channel overlay (layer 0 registers) */
	uint32_t ovl_base;			/* offset of the overlay from the engine base */
	uint16_t layer_attr, layer_size, layer_coor, layer_pitch, layer_laddr, layer_bot_laddr, layer_fcolor;
	uint16_t misc_haddr, misc_bot_haddr, misc_ovl_size;	/* offsets from ovl_base + misc_off */
	uint16_t misc_off;
	uint32_t attr_en;
	uint8_t attr_alpha_mode_shift, attr_fmt_shift, attr_alpha_shift;
	uint8_t alpha_mode_pixel, alpha_mode_layer;	/* ARGB: per pixel, others: layer alpha */
	uint8_t fmt[4];				/* indexed by sunxi_de_format_t */
	uint8_t fmt_cpp[4];			/* bytes per pixel, same index */
	uint8_t nlayers;			/* layers of the overlay (all disabled but 0) */
	uint16_t layer_stride;

	/* blender */
	uint32_t bld_base;
	uint16_t bld_fill_ctl;			/* from bld_base */
	uint8_t bld_pipe_fill_bit, bld_pipe_en_bit;	/* bit of pipe 0 in fill_ctl */
	uint16_t bld_pipe_fcolor, bld_pipe_size, bld_pipe_offset;	/* pipe 0, from bld_base */
	uint16_t bld_pipe_stride;
	uint16_t bld_ctl_block;			/* from bld_base */
	uint16_t bld_route, bld_premul, bld_bkcolor, bld_out_size, bld_mode;	/* from bld_ctl_block */
	uint8_t bld_route_width;		/* bits per pipe in the route register */
	uint8_t bld_npipes;
	uint32_t bld_src_over;			/* blend mode word */
	uint16_t bld_ck_block;			/* from bld_base */
	uint16_t bld_ck_ctl, bld_out_ctl;	/* from bld_ck_block */
	uint8_t chn_port;			/* blender route value of this UI channel */
	uint8_t size_minus_one;			/* sizes written as value - 1 */

	/* stages that stay in bypass: written with the given values at init */
	struct sunxi_de_bypass bypass[SUNXI_DE_MAX_BYPASS];
	uint8_t nbypass;
};

extern const struct sunxi_de_variant sunxi_de_variant_sun252iw2;

#endif /* __SUNXI_DE_REGS_H__ */
