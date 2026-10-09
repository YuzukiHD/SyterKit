// SPDX-License-Identifier: GPL-2.0+
/*
 * Display engine, UI passthrough: the engine is only used as a frame
 * buffer reader. One UI channel (layer 0, no scaling) feeds blender pipe 0
 * at the output size, everything behind the blender is bypassed, and the
 * result goes straight into the TCON. No composition takes place.
 */
#include <io.h>
#include <log.h>
#include <string.h>
#include <timer.h>

#include <dt2c/driver.h>
#include <drivers/display/display.h>

#include "de-regs.h"

#undef pr_fmt
#define pr_fmt(fmt) "de: " fmt

static const struct sunxi_de_variant *const sunxi_de_variants[] = {
	&sunxi_de_variant_sun252iw2,
};

const struct sunxi_de_variant *sunxi_de_variant_find(const char *name)
{
	unsigned int i;

	if (name == NULL)
		return NULL;
	for (i = 0; i < sizeof(sunxi_de_variants) / sizeof(sunxi_de_variants[0]); i++)
		if (strcmp(name, sunxi_de_variants[i]->name) == 0)
			return sunxi_de_variants[i];
	return NULL;
}

static inline uint32_t de_rd(const sunxi_de_t *de, uint32_t off)
{
	return readl(de->res.base + off);
}

static inline void de_wr(const sunxi_de_t *de, uint32_t off, uint32_t val)
{
	writel(val, de->res.base + off);
}

static inline void de_upd(const sunxi_de_t *de, uint32_t off, uint32_t mask, uint32_t val)
{
	clrsetbits_le32(de->res.base + off, mask, val);
}

static uint32_t de_size_word(const struct sunxi_de_variant *v, uint32_t w, uint32_t h)
{
	uint32_t adj = v->size_minus_one ? 1U : 0U;

	return ((((w - adj) & v->size_w_mask) << v->size_w_shift) | (((h - adj) & v->size_h_mask) << v->size_h_shift));
}

/* A layer size word keeps the usual layout: width in the low half, height in the high half. */
static uint32_t de_layer_size_word(uint32_t w, uint32_t h)
{
	return (((h - 1U) & 0x1fffU) << 16) | ((w - 1U) & 0x1fffU);
}

static void de_core_enable(sunxi_de_t *de, bool on)
{
	const struct sunxi_de_variant *v = de->var;
	uint32_t core = 1U << v->core_shift;

	if (on) {
		/* release the mixer core, then open its bus and clock gates */
		de_upd(de, v->top_reset, core, core);
		de_upd(de, v->top_ahb_gate, core, core);
		de_upd(de, v->top_mbus_gate, core, core);
		de_upd(de, v->top_clk_gate, core, core);
		de_wr(de, v->glb_size, de_size_word(v, de->width, de->height));
		de_wr(de, v->glb_ctl, v->glb_ctl_enable);
	} else {
		de_wr(de, v->glb_ctl, 0);
		de_upd(de, v->top_clk_gate, core, 0);
		de_upd(de, v->top_mbus_gate, core, 0);
		de_upd(de, v->top_ahb_gate, core, 0);
		de_upd(de, v->top_reset, core, 0);
	}
}

/* one blender pipe at the output size, fed by the UI channel, opaque black background */
static void de_blender_setup(sunxi_de_t *de)
{
	const struct sunxi_de_variant *v = de->var;
	uint32_t base = v->bld_base;
	uint32_t ctl = base + v->bld_ctl_block;
	uint32_t ck = base + v->bld_ck_block;
	uint32_t bg = 0xff000000U;
	uint32_t size = ((de->height - 1U) << 16) | (de->width - 1U);
	unsigned int p;

	for (p = 0; p < v->bld_npipes; p++) {
		de_wr(de, base + v->bld_pipe_fcolor + p * v->bld_pipe_stride, bg);
		if (p == 0) {
			de_wr(de, base + v->bld_pipe_size, size);
			de_wr(de, base + v->bld_pipe_offset, 0);
		}
	}
	de_wr(de, base + v->bld_fill_ctl, (1U << v->bld_pipe_fill_bit) | (1U << v->bld_pipe_en_bit));

	de_wr(de, ctl + v->bld_route, (uint32_t)v->chn_port); /* pipe 0 */
	de_wr(de, ctl + v->bld_premul, 0);
	de_wr(de, ctl + v->bld_bkcolor, bg);
	de_wr(de, ctl + v->bld_out_size, size);
	for (p = 0; p < v->bld_npipes; p++)
		de_wr(de, ctl + v->bld_mode + p * 4U, v->bld_src_over);

	/* no colour key, progressive, non pre-multiplied output */
	de_wr(de, ck + v->bld_ck_ctl, 0);
	de_wr(de, ck + v->bld_out_ctl, 0);
}

static void de_layers_off(sunxi_de_t *de)
{
	const struct sunxi_de_variant *v = de->var;
	unsigned int i;

	for (i = 0; i < v->nlayers; i++)
		de_wr(de, v->ovl_base + i * v->layer_stride + v->layer_attr, 0);
}

int sunxi_de_init(sunxi_de_t *de, uint32_t width, uint32_t height)
{
	const struct sunxi_de_variant *v;
	uint32_t rate, set;
	unsigned int i;

	if (de == NULL || de->var == NULL || de->clk == NULL || width == 0U || height == 0U ||
		width > de->var->size_w_mask || height > de->var->size_h_mask)
		return DRIVER_ERROR_INVALID;
	v = de->var;
	de->width = width;
	de->height = height;
	de->port = v->port;

	pr_debug("bus enable\n");
	sunxi_disp_res_bus_enable(&de->res, true);
	rate = de->res.mod_rate != 0U ? de->res.mod_rate : v->default_mod_hz;
	set = sunxi_disp_modclk_set(de->clk, &de->res, &v->modclk, rate);
	if (set == 0U) {
		pr_err("module clock setup failed\n");
		sunxi_disp_res_bus_enable(&de->res, false);
		return DRIVER_ERROR_INVALID;
	}
	pr_debug("module clock %u Hz\n", (unsigned int)set);

	de_core_enable(de, true);
	pr_debug("core enabled\n");
	de_layers_off(de);
	de_blender_setup(de);
	pr_debug("blender set; fcolor rd %08x route rd %08x glb_ctl %08x\n",
		(unsigned int)de_rd(de, v->bld_base + v->bld_pipe_fcolor),
		(unsigned int)de_rd(de, v->bld_base + v->bld_ctl_block + v->bld_route),
		(unsigned int)de_rd(de, v->glb_ctl));
	for (i = 0; i < v->nbypass; i++)
		de_wr(de, v->bypass[i].offset, v->bypass[i].value);

	de->powered = true;
	de->enabled = false;
	return DRIVER_OK;
}

int sunxi_de_ui_set_fb(
	sunxi_de_t *de, uintptr_t addr, uint32_t width, uint32_t height, uint32_t stride_bytes, sunxi_de_format_t fmt)
{
	const struct sunxi_de_variant *v;
	uint32_t l, attr, alpha_mode, ovl_size;

	if (de == NULL || de->var == NULL || !de->powered || (unsigned int)fmt >= 4U)
		return DRIVER_ERROR_INVALID;
	v = de->var;
	if (width == 0U || height == 0U || width > de->width || height > de->height ||
		stride_bytes < width * v->fmt_cpp[fmt] || v->fmt_cpp[fmt] == 0U)
		return DRIVER_ERROR_INVALID;

	l = v->ovl_base;
	alpha_mode = fmt == SUNXI_DE_FMT_ARGB8888 ? v->alpha_mode_pixel : v->alpha_mode_layer;
	attr = (0xffU << v->attr_alpha_shift) | ((uint32_t)alpha_mode << v->attr_alpha_mode_shift) |
	       ((uint32_t)v->fmt[fmt] << v->attr_fmt_shift) | (de->enabled ? v->attr_en : 0U);
	ovl_size = de_layer_size_word(width, height);

	de_wr(de, l + v->layer_size, ovl_size);
	de_wr(de, l + v->layer_coor, 0);
	de_wr(de, l + v->layer_pitch, stride_bytes);
	de_wr(de, l + v->layer_laddr, (uint32_t)addr);
	de_wr(de, l + v->layer_bot_laddr, 0);
	de_wr(de, l + v->layer_fcolor, 0);
	/* bits 39:32 of the address; the layer index selects the byte */
	de_wr(de, l + v->misc_off + v->misc_haddr, (uint32_t)(((uint64_t)addr >> 32) & 0xffU));
	de_wr(de, l + v->misc_off + v->misc_bot_haddr, 0);
	de_wr(de, l + v->misc_off + v->misc_ovl_size, ovl_size);
	de_wr(de, l + v->layer_attr, attr);
	return DRIVER_OK;
}

int sunxi_de_enable(sunxi_de_t *de, bool on)
{
	const struct sunxi_de_variant *v;

	if (de == NULL || de->var == NULL || !de->powered)
		return DRIVER_ERROR_INVALID;
	v = de->var;
	de_upd(de, v->ovl_base + v->layer_attr, v->attr_en, on ? v->attr_en : 0U);
	de->enabled = on;
	return DRIVER_OK;
}

void sunxi_de_deinit(sunxi_de_t *de)
{
	if (de == NULL || de->var == NULL || !de->powered)
		return;
	de_layers_off(de);
	de_core_enable(de, false);
	sunxi_disp_modclk_gate(&de->res, false);
	sunxi_disp_res_bus_enable(&de->res, false);
	de->powered = false;
	de->enabled = false;
}

void sunxi_de_dump(sunxi_de_t *de)
{
	const struct sunxi_de_variant *v;
	uint32_t l;

	if (de == NULL || de->var == NULL || !de->powered) {
		pr_debug("powered off\n");
		return;
	}
	v = de->var;
	l = v->ovl_base;
	pr_debug("top: reset %08x ahb %08x mbus %08x clk %08x\n", de_rd(de, v->top_reset), de_rd(de, v->top_ahb_gate),
		de_rd(de, v->top_mbus_gate), de_rd(de, v->top_clk_gate));
	pr_debug("mixer: ctl %08x size %08x\n", de_rd(de, v->glb_ctl), de_rd(de, v->glb_size));
	pr_debug("layer0: attr %08x size %08x pitch %08x addr %08x ovl %08x\n", de_rd(de, l + v->layer_attr),
		de_rd(de, l + v->layer_size), de_rd(de, l + v->layer_pitch), de_rd(de, l + v->layer_laddr),
		de_rd(de, l + v->misc_off + v->misc_ovl_size));
	pr_debug("blender: fill %08x route %08x out %08x pipe0 %08x\n", de_rd(de, v->bld_base + v->bld_fill_ctl),
		de_rd(de, v->bld_base + v->bld_ctl_block + v->bld_route),
		de_rd(de, v->bld_base + v->bld_ctl_block + v->bld_out_size), de_rd(de, v->bld_base + v->bld_pipe_size));
}

DT2C_DRIVER_COMPAT("allwinner,sunxi-de");
