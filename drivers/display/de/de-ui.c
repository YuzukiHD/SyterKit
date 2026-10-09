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

static inline uint32_t de_read_reg(const sunxi_de_t *de, uint32_t off)
{
	return readl(de->res.base + off);
}

static inline void de_write_reg(const sunxi_de_t *de, uint32_t off, uint32_t val)
{
	writel(val, de->res.base + off);
}

static inline void de_modify_reg(const sunxi_de_t *de, uint32_t off, uint32_t mask, uint32_t val)
{
	clrsetbits_le32(de->res.base + off, mask, val);
}

static uint32_t de_size_word(const struct sunxi_de_variant *variant, uint32_t width, uint32_t height)
{
	uint32_t adj = variant->size_minus_one ? 1U : 0U;

	return ((((width - adj) & variant->size_w_mask) << variant->size_w_shift) |
		(((height - adj) & variant->size_h_mask) << variant->size_h_shift));
}

/* A layer size word keeps the usual layout: width in the low half, height in the high half. */
static uint32_t de_layer_size_word(uint32_t width, uint32_t height)
{
	return (((height - 1U) & 0x1fffU) << 16) | ((width - 1U) & 0x1fffU);
}

static void de_core_enable(sunxi_de_t *de, bool on)
{
	const struct sunxi_de_variant *variant = de->var;
	uint32_t core = 1U << variant->core_shift;

	if (on) {
		/* release the mixer core, then open its bus and clock gates */
		de_modify_reg(de, variant->top_reset, core, core);
		de_modify_reg(de, variant->top_ahb_gate, core, core);
		de_modify_reg(de, variant->top_mbus_gate, core, core);
		de_modify_reg(de, variant->top_clk_gate, core, core);
		de_write_reg(de, variant->glb_size, de_size_word(variant, de->width, de->height));
		de_write_reg(de, variant->glb_ctl, variant->glb_ctl_enable);
	} else {
		de_write_reg(de, variant->glb_ctl, 0);
		de_modify_reg(de, variant->top_clk_gate, core, 0);
		de_modify_reg(de, variant->top_mbus_gate, core, 0);
		de_modify_reg(de, variant->top_ahb_gate, core, 0);
		de_modify_reg(de, variant->top_reset, core, 0);
	}
}

/* one blender pipe at the output size, fed by the UI channel, opaque black background */
static void de_blender_setup(sunxi_de_t *de)
{
	const struct sunxi_de_variant *variant = de->var;
	uint32_t base = variant->bld_base;
	uint32_t ctl = base + variant->bld_ctl_block;
	uint32_t ck = base + variant->bld_ck_block;
	uint32_t bg = 0xff000000U;
	uint32_t size = ((de->height - 1U) << 16) | (de->width - 1U);
	unsigned int pipe;

	for (pipe = 0; pipe < variant->bld_npipes; pipe++) {
		de_write_reg(de, base + variant->bld_pipe_fcolor + pipe * variant->bld_pipe_stride, bg);
		if (pipe == 0) {
			de_write_reg(de, base + variant->bld_pipe_size, size);
			de_write_reg(de, base + variant->bld_pipe_offset, 0);
		}
	}
	de_write_reg(de, base + variant->bld_fill_ctl,
		(1U << variant->bld_pipe_fill_bit) | (1U << variant->bld_pipe_en_bit));

	de_write_reg(de, ctl + variant->bld_route, (uint32_t)variant->chn_port); /* pipe 0 */
	de_write_reg(de, ctl + variant->bld_premul, 0);
	de_write_reg(de, ctl + variant->bld_bkcolor, bg);
	de_write_reg(de, ctl + variant->bld_out_size, size);
	for (pipe = 0; pipe < variant->bld_npipes; pipe++)
		de_write_reg(de, ctl + variant->bld_mode + pipe * 4U, variant->bld_src_over);

	/* no colour key, progressive, non pre-multiplied output */
	de_write_reg(de, ck + variant->bld_ck_ctl, 0);
	de_write_reg(de, ck + variant->bld_out_ctl, 0);
}

static void de_layers_off(sunxi_de_t *de)
{
	const struct sunxi_de_variant *variant = de->var;
	unsigned int i;

	for (i = 0; i < variant->nlayers; i++)
		de_write_reg(de, variant->ovl_base + i * variant->layer_stride + variant->layer_attr, 0);
}

int sunxi_de_init(sunxi_de_t *de, uint32_t width, uint32_t height)
{
	const struct sunxi_de_variant *variant;
	uint32_t rate, set;
	unsigned int i;

	if (de == NULL || de->var == NULL || de->clk == NULL || width == 0U || height == 0U ||
		width > de->var->size_w_mask || height > de->var->size_h_mask)
		return DRIVER_ERROR_INVALID;
	variant = de->var;
	de->width = width;
	de->height = height;
	de->port = variant->port;

	pr_debug("bus enable\n");
	sunxi_disp_res_bus_enable(&de->res, true);
	rate = de->res.mod_rate != 0U ? de->res.mod_rate : variant->default_mod_hz;
	set = sunxi_disp_modclk_set(de->clk, &de->res, &variant->modclk, rate);
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
		(unsigned int)de_read_reg(de, variant->bld_base + variant->bld_pipe_fcolor),
		(unsigned int)de_read_reg(de, variant->bld_base + variant->bld_ctl_block + variant->bld_route),
		(unsigned int)de_read_reg(de, variant->glb_ctl));
	for (i = 0; i < variant->nbypass; i++)
		de_write_reg(de, variant->bypass[i].offset, variant->bypass[i].value);

	de->powered = true;
	de->enabled = false;
	return DRIVER_OK;
}

int sunxi_de_ui_set_fb(
	sunxi_de_t *de, uintptr_t addr, uint32_t width, uint32_t height, uint32_t stride_bytes, sunxi_de_format_t fmt)
{
	const struct sunxi_de_variant *variant;
	uint32_t layer_base, attr, alpha_mode, ovl_size;

	if (de == NULL || de->var == NULL || !de->powered || (unsigned int)fmt >= 4U)
		return DRIVER_ERROR_INVALID;
	variant = de->var;
	if (width == 0U || height == 0U || width > de->width || height > de->height ||
		stride_bytes < width * variant->fmt_cpp[fmt] || variant->fmt_cpp[fmt] == 0U)
		return DRIVER_ERROR_INVALID;

	layer_base = variant->ovl_base;
	alpha_mode = fmt == SUNXI_DE_FMT_ARGB8888 ? variant->alpha_mode_pixel : variant->alpha_mode_layer;
	attr = (0xffU << variant->attr_alpha_shift) | ((uint32_t)alpha_mode << variant->attr_alpha_mode_shift) |
	       ((uint32_t)variant->fmt[fmt] << variant->attr_fmt_shift) | (de->enabled ? variant->attr_en : 0U);
	ovl_size = de_layer_size_word(width, height);

	de_write_reg(de, layer_base + variant->layer_size, ovl_size);
	de_write_reg(de, layer_base + variant->layer_coor, 0);
	de_write_reg(de, layer_base + variant->layer_pitch, stride_bytes);
	de_write_reg(de, layer_base + variant->layer_laddr, (uint32_t)addr);
	de_write_reg(de, layer_base + variant->layer_bot_laddr, 0);
	de_write_reg(de, layer_base + variant->layer_fcolor, 0);
	/* bits 39:32 of the address; the layer index selects the byte */
	de_write_reg(
		de, layer_base + variant->misc_off + variant->misc_haddr, (uint32_t)(((uint64_t)addr >> 32) & 0xffU));
	de_write_reg(de, layer_base + variant->misc_off + variant->misc_bot_haddr, 0);
	de_write_reg(de, layer_base + variant->misc_off + variant->misc_ovl_size, ovl_size);
	de_write_reg(de, layer_base + variant->layer_attr, attr);
	return DRIVER_OK;
}

int sunxi_de_enable(sunxi_de_t *de, bool on)
{
	const struct sunxi_de_variant *variant;

	if (de == NULL || de->var == NULL || !de->powered)
		return DRIVER_ERROR_INVALID;
	variant = de->var;
	de_modify_reg(de, variant->ovl_base + variant->layer_attr, variant->attr_en, on ? variant->attr_en : 0U);
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
	const struct sunxi_de_variant *variant;
	uint32_t layer_base;

	if (de == NULL || de->var == NULL || !de->powered) {
		pr_debug("powered off\n");
		return;
	}
	variant = de->var;
	layer_base = variant->ovl_base;
	pr_debug("top: reset %08x ahb %08x mbus %08x clk %08x\n", de_read_reg(de, variant->top_reset),
		de_read_reg(de, variant->top_ahb_gate), de_read_reg(de, variant->top_mbus_gate),
		de_read_reg(de, variant->top_clk_gate));
	pr_debug("mixer: ctl %08x size %08x\n", de_read_reg(de, variant->glb_ctl), de_read_reg(de, variant->glb_size));
	pr_debug("layer0: attr %08x size %08x pitch %08x addr %08x ovl %08x\n",
		de_read_reg(de, layer_base + variant->layer_attr), de_read_reg(de, layer_base + variant->layer_size),
		de_read_reg(de, layer_base + variant->layer_pitch), de_read_reg(de, layer_base + variant->layer_laddr),
		de_read_reg(de, layer_base + variant->misc_off + variant->misc_ovl_size));
	pr_debug("blender: fill %08x route %08x out %08x pipe0 %08x\n",
		de_read_reg(de, variant->bld_base + variant->bld_fill_ctl),
		de_read_reg(de, variant->bld_base + variant->bld_ctl_block + variant->bld_route),
		de_read_reg(de, variant->bld_base + variant->bld_ctl_block + variant->bld_out_size),
		de_read_reg(de, variant->bld_base + variant->bld_pipe_size));
}

DT2C_DRIVER_COMPAT("allwinner,sunxi-de");
