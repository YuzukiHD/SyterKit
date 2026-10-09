/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __DT_COMPATIBLE_DISPLAY_DT_H__
#define __DT_COMPATIBLE_DISPLAY_DT_H__

#include <stdbool.h>
#include <stdint.h>

#include <driver.h>

#ifndef UINT8_MAX
#define UINT8_MAX 0xffU
#endif
#include <drivers/display/display.h>
#include <dt-compatible/de-dt.h>
#include <dt-compatible/display-res-dt.h>
#include <dt-compatible/dphy-dt.h>
#include <dt-compatible/dsi-dt.h>
#include <dt-compatible/gpio-dt.h>
#include <dt-compatible/pinctrl-dt.h>
#include <dt-compatible/pwm-dt.h>
#include <dt-compatible/tcon-dt.h>

#define SUNXI_DISPLAY_COMPATIBLE   "allwinner,sunxi-display"
#define SUNXI_DISPLAY_RGB_COMPATIBLE  "allwinner,sunxi-rgb"
#define SUNXI_DISPLAY_LVDS_COMPATIBLE "allwinner,sunxi-lvds"
#define SUNXI_DISPLAY_PANEL_COMPATIBLE "allwinner,sunxi-panel"
#define SUNXI_DISPLAY_BACKLIGHT_COMPATIBLE "allwinner,sunxi-backlight"

/* Defined by display.c: chip description by the allwinner,variant name. */
const struct sunxi_disp_soc *sunxi_disp_soc_find(const char *name);

static inline __attribute__((always_inline)) uint32_t sunxi_disp_dt_u32(int node, const char *name, uint32_t fallback)
{
	const dt2c_fdt32_t *cells = syterkit_dt_cells(node, name, 1);

	return cells != NULL ? dt2c_fdt32_to_cpu(cells[0]) : fallback;
}

static inline __attribute__((always_inline)) bool sunxi_disp_dt_flag(int node, const char *name)
{
	int length;

	return dt2c_fdt_getprop(DT2C_FDT_COMPILED_TREE, node, name, &length) != NULL;
}

/* node a phandle property points to, or a negative value */
static inline __attribute__((always_inline)) int sunxi_disp_dt_ref(int node, const char *name)
{
	const dt2c_fdt32_t *cells = syterkit_dt_cells(node, name, 1);

	if (cells == NULL)
		return -DT2C_FDT_ERR_BADVALUE;
	return dt2c_fdt_node_offset_by_phandle(DT2C_FDT_COMPILED_TREE, dt2c_fdt32_to_cpu(cells[0]));
}

static inline __attribute__((always_inline)) bool sunxi_disp_dt_str(
	int node, const char *name, const char *value, size_t value_len)
{
	int length;
	const char *s = (const char *)dt2c_fdt_getprop(DT2C_FDT_COMPILED_TREE, node, name, &length);

	return syterkit_dt_string_equal(s, length, value, value_len);
}

static inline __attribute__((always_inline)) sunxi_disp_bus_t sunxi_disp_dt_bus(int node, const char *name)
{
	if (sunxi_disp_dt_str(node, name, "rgb666", 6))
		return SUNXI_DISP_BUS_RGB666;
	if (sunxi_disp_dt_str(node, name, "rgb565", 6))
		return SUNXI_DISP_BUS_RGB565;
	return SUNXI_DISP_BUS_RGB888;
}

static inline __attribute__((always_inline)) sunxi_dsi_format_t sunxi_disp_dt_dsi_format(int node)
{
	if (sunxi_disp_dt_str(node, "allwinner,dsi-format", "rgb666", 6))
		return SUNXI_DSI_FMT_RGB666;
	if (sunxi_disp_dt_str(node, "allwinner,dsi-format", "rgb666-packed", 13))
		return SUNXI_DSI_FMT_RGB666_PACKED;
	if (sunxi_disp_dt_str(node, "allwinner,dsi-format", "rgb565", 6))
		return SUNXI_DSI_FMT_RGB565;
	return SUNXI_DSI_FMT_RGB888;
}

static inline __attribute__((always_inline)) sunxi_disp_seq_t sunxi_disp_dt_seq(int node, const char *name)
{
	sunxi_disp_seq_t seq = { 0 };
	int length;
	const dt2c_fdt32_t *cells = (const dt2c_fdt32_t *)dt2c_fdt_getprop(DT2C_FDT_COMPILED_TREE, node, name, &length);

	if (cells != NULL && length > 0 && length % (int)sizeof(*cells) == 0) {
		seq.cells = (const uint32_t *)cells;
		seq.count = (uint32_t)length / sizeof(*cells);
	}
	return seq;
}

static inline __attribute__((always_inline)) int sunxi_panel_dt_read_config(
	sunxi_panel_t *panel, int node)
{
	sunxi_disp_timing_t *t = &panel->timing;
	int ctrl;

	if (panel == NULL || node < 0 || !syterkit_dt_node_available(node) ||
		dt2c_fdt_node_check_compatible(DT2C_FDT_COMPILED_TREE, node, SUNXI_DISPLAY_PANEL_COMPATIBLE) != 0)
		return DRIVER_ERROR_INVALID;
	*panel = (sunxi_panel_t){ 0 };

	t->pixel_clock_hz = sunxi_disp_dt_u32(node, "clock-frequency", 0U);
	t->hactive = (uint16_t)sunxi_disp_dt_u32(node, "hactive", 0U);
	t->vactive = (uint16_t)sunxi_disp_dt_u32(node, "vactive", 0U);
	t->hfront_porch = (uint16_t)sunxi_disp_dt_u32(node, "hfront-porch", 0U);
	t->hback_porch = (uint16_t)sunxi_disp_dt_u32(node, "hback-porch", 0U);
	t->hsync_len = (uint16_t)sunxi_disp_dt_u32(node, "hsync-len", 0U);
	t->vfront_porch = (uint16_t)sunxi_disp_dt_u32(node, "vfront-porch", 0U);
	t->vback_porch = (uint16_t)sunxi_disp_dt_u32(node, "vback-porch", 0U);
	t->vsync_len = (uint16_t)sunxi_disp_dt_u32(node, "vsync-len", 0U);
	t->hsync_active = (uint8_t)sunxi_disp_dt_u32(node, "hsync-active", 0U);
	t->vsync_active = (uint8_t)sunxi_disp_dt_u32(node, "vsync-active", 0U);
	t->de_active = (uint8_t)sunxi_disp_dt_u32(node, "de-active", 1U);
	t->pixelclk_active = (uint8_t)sunxi_disp_dt_u32(node, "pixelclk-active", 1U);
	if (t->pixel_clock_hz == 0U || t->hactive == 0U || t->vactive == 0U || t->hsync_len == 0U || t->vsync_len == 0U)
		return DRIVER_ERROR_INVALID;
	t->htotal = (uint16_t)(t->hactive + t->hfront_porch + t->hsync_len + t->hback_porch);
	t->vtotal = (uint16_t)(t->vactive + t->vfront_porch + t->vsync_len + t->vback_porch);

	panel->bus = sunxi_disp_dt_bus(node, "allwinner,bus-format");
	panel->dsi_lanes = (uint8_t)sunxi_disp_dt_u32(node, "allwinner,dsi-lanes", 0U);
	panel->dsi_channel = (uint8_t)sunxi_disp_dt_u32(node, "allwinner,dsi-channel", 0U);
	panel->dsi_format = sunxi_disp_dt_dsi_format(node);
	panel->dsi_mode_flags = sunxi_disp_dt_u32(node, "allwinner,dsi-mode-flags", SUNXI_DSI_MODE_VIDEO);
	panel->enable_delay_ms = sunxi_disp_dt_u32(node, "enable-delay-ms", 0U);
	panel->width_mm = (uint16_t)sunxi_disp_dt_u32(node, "width-mm", 0U);
	panel->height_mm = (uint16_t)sunxi_disp_dt_u32(node, "height-mm", 0U);
	panel->power_on = sunxi_disp_dt_seq(node, "power-on-sequence");
	panel->power_off = sunxi_disp_dt_seq(node, "power-off-sequence");
	panel->init = sunxi_disp_dt_seq(node, "init-sequence");
	panel->exit = sunxi_disp_dt_seq(node, "exit-sequence");

	ctrl = sunxi_disp_dt_ref(node, "allwinner,gpio-controller");
	if (ctrl >= 0) {
		sunxi_gpio_t gpio;

		if (sunxi_gpio_dt_read_config(&gpio, ctrl) == DRIVER_OK) {
			panel->gpio_base = gpio.base;
			panel->gpio_bank_base = gpio.bank_base;
		}
	}
	return DRIVER_OK;
}

static inline __attribute__((always_inline)) int sunxi_backlight_dt_read_config(
	sunxi_backlight_t *bl, int node)
{
	int pwm_node;

	if (bl == NULL || node < 0 || !syterkit_dt_node_available(node) ||
		dt2c_fdt_node_check_compatible(DT2C_FDT_COMPILED_TREE, node, SUNXI_DISPLAY_BACKLIGHT_COMPATIBLE) != 0)
		return DRIVER_ERROR_INVALID;
	*bl = (sunxi_backlight_t){ 0 };
	bl->level = (uint8_t)sunxi_disp_dt_u32(node, "allwinner,brightness", 100U);
	if (bl->level > 100U)
		bl->level = 100U;

	pwm_node = sunxi_disp_dt_ref(node, "allwinner,pwm");
	if (pwm_node >= 0) {
		if (sunxi_pwm_dt_read_config(&bl->pwm, pwm_node) != DRIVER_OK)
			return DRIVER_ERROR_INVALID;
		bl->type = SUNXI_BACKLIGHT_PWM;
		bl->pwm_channel = (uint8_t)sunxi_disp_dt_u32(node, "allwinner,pwm-channel", 0U);
		bl->pwm_period_ns = sunxi_disp_dt_u32(node, "allwinner,pwm-period-ns", 1000000U);
		bl->pwm_active_high = (uint8_t)sunxi_disp_dt_u32(node, "allwinner,active-high", 1U);
		return bl->pwm_period_ns != 0U ? DRIVER_OK : DRIVER_ERROR_INVALID;
	}
	if (sunxi_gpio_dt_read_property(&bl->gpio, node, "enable-gpios")) {
		bl->type = SUNXI_BACKLIGHT_GPIO;
		bl->gpio.mux = GPIO_OUTPUT;
		bl->gpio_active_high = (uint8_t)sunxi_disp_dt_u32(node, "allwinner,active-high", 1U);
		return DRIVER_OK;
	}
	return DRIVER_ERROR_INVALID;
}

/* pins of the pinctrl-0 group of @node, applied before the output starts */
static inline __attribute__((always_inline)) int sunxi_display_dt_read_pins(sunxi_display_t *disp, int node)
{
	sunxi_gpio_t controller;
	size_t cells;
	const dt2c_fdt32_t *pins = syterkit_dt_pinctrl(node, &cells, &controller);
	size_t i;

	disp->pin_count = 0U;
	disp->pin_drive = -1;
	if (pins == NULL)
		return DRIVER_OK;	/* pads without a pinctrl group (e.g. dedicated LVDS pads) */
	if (cells / 3U > SUNXI_DISP_MAX_PINS)
		return DRIVER_ERROR_INVALID;
	for (i = 0; i < cells / 3U; i++)
		if (!sunxi_gpio_dt_read_pin(&disp->pins[i], &controller, pins + i * 3U))
			return DRIVER_ERROR_INVALID;
	disp->pin_count = (uint8_t)(cells / 3U);
	if (sunxi_disp_dt_u32(node, "allwinner,drive-level", 0xffU) != 0xffU)
		disp->pin_drive = (int8_t)sunxi_disp_dt_u32(node, "allwinner,drive-level", 0U);
	return DRIVER_OK;
}

static inline __attribute__((always_inline)) int sunxi_display_dt_read_config(sunxi_display_t *disp, int node)
{
	const dt2c_fdt32_t *pll_video, *pll_peri;
	const char *variant;
	int length, n, out, panel;

	if (disp == NULL || node < 0 || !syterkit_dt_node_available(node) ||
		dt2c_fdt_node_check_compatible(DT2C_FDT_COMPILED_TREE, node, SUNXI_DISPLAY_COMPATIBLE) != 0)
		return DRIVER_ERROR_INVALID;
	*disp = (sunxi_display_t){ 0 };
	disp->dt_node = node;

	variant = (const char *)dt2c_fdt_getprop(DT2C_FDT_COMPILED_TREE, node, "allwinner,variant", &length);
	pll_video = syterkit_dt_cells(node, "allwinner,pll-video", 1);
	pll_peri = syterkit_dt_cells(node, "allwinner,pll-peri", 1);
	if (variant == NULL || length <= 1 || variant[length - 1] != '\0' || pll_video == NULL || pll_peri == NULL)
		return DRIVER_ERROR_INVALID;
	disp->clk.soc = sunxi_disp_soc_find(variant);
	if (disp->clk.soc == NULL)
		return DRIVER_ERROR_INVALID;
	disp->clk.pll_video_reg = (uintptr_t)dt2c_fdt32_to_cpu(pll_video[0]);
	disp->clk.pll_peri_reg = (uintptr_t)dt2c_fdt32_to_cpu(pll_peri[0]);

	/* display engine and timing controller */
	if (sunxi_de_dt_read_config(&disp->de, sunxi_disp_dt_ref(node, "allwinner,de")) != DRIVER_OK ||
		sunxi_tcon_dt_read_config(&disp->tcon, sunxi_disp_dt_ref(node, "allwinner,tcon")) != DRIVER_OK)
		return DRIVER_ERROR_INVALID;
	disp->de.clk = &disp->clk;
	disp->tcon.clk = &disp->clk;
	disp->tcon.de_port = disp->de.port;
	n = sunxi_disp_dt_ref(node, "allwinner,tcon-top");
	if (n >= 0) {
		if (sunxi_tcon_top_dt_read_config(&disp->top, n) != DRIVER_OK)
			return DRIVER_ERROR_INVALID;
		disp->has_top = true;
		disp->tcon.top = &disp->top;
	}

	/* the output: rgb, lvds or dsi */
	out = sunxi_disp_dt_ref(node, "allwinner,output");
	if (out < 0 || !syterkit_dt_node_available(out))
		return DRIVER_ERROR_INVALID;
	if (dt2c_fdt_node_check_compatible(DT2C_FDT_COMPILED_TREE, out, SUNXI_DISPLAY_RGB_COMPATIBLE) == 0) {
		disp->iface = SUNXI_DISP_IF_RGB;
		disp->if_cfg.hv_mode = (uint8_t)sunxi_disp_dt_u32(out, "allwinner,hv-mode", 0U);
		disp->if_cfg.srgb_seq = (uint8_t)sunxi_disp_dt_u32(out, "allwinner,srgb-seq", 0U);
		disp->if_cfg.syuv_seq = (uint8_t)sunxi_disp_dt_u32(out, "allwinner,syuv-seq", 0U);
		disp->if_cfg.syuv_fdly = (uint8_t)sunxi_disp_dt_u32(out, "allwinner,syuv-fdly", 0U);
		disp->if_cfg.rgb_swap = (uint8_t)sunxi_disp_dt_u32(out, "allwinner,rgb-swap", 0U);
		disp->if_cfg.rb_swap = (uint8_t)sunxi_disp_dt_u32(out, "allwinner,rb-swap", 0U);
		disp->if_cfg.clk_phase = (uint8_t)sunxi_disp_dt_u32(out, "allwinner,clk-phase", 0U);
		disp->if_cfg.io_adjust = sunxi_disp_dt_u32(out, "allwinner,io-adjust", 0U);
	} else if (dt2c_fdt_node_check_compatible(DT2C_FDT_COMPILED_TREE, out, SUNXI_DISPLAY_LVDS_COMPATIBLE) == 0) {
		const dt2c_fdt32_t *rst = syterkit_dt_cells(out, "allwinner,reset", 2);

		disp->iface = SUNXI_DISP_IF_LVDS;
		disp->if_cfg.lvds_dual_link = sunxi_disp_dt_flag(out, "allwinner,dual-link");
		disp->if_cfg.lvds_jeida = sunxi_disp_dt_flag(out, "allwinner,jeida");
		if (rst != NULL) {
			disp->lvds_reset.reg = (uintptr_t)dt2c_fdt32_to_cpu(rst[0]);
			disp->lvds_reset.bit = (uint8_t)dt2c_fdt32_to_cpu(rst[1]);
		}
	} else if (dt2c_fdt_node_check_compatible(DT2C_FDT_COMPILED_TREE, out, SUNXI_DSI_COMPATIBLE) == 0) {
		disp->iface = SUNXI_DISP_IF_DSI;
		if (sunxi_dsi_dt_read_config(&disp->dsi, out) != DRIVER_OK)
			return DRIVER_ERROR_INVALID;
		disp->dsi.clk = &disp->clk;
		disp->dsi.phy = &disp->dphy;
		disp->if_cfg.dsi_id = disp->dsi.id;
	} else {
		return DRIVER_ERROR_INVALID;
	}
	if (sunxi_display_dt_read_pins(disp, out) != DRIVER_OK)
		return DRIVER_ERROR_INVALID;

	if (disp->iface != SUNXI_DISP_IF_RGB) {
		n = sunxi_disp_dt_ref(out, "allwinner,phy");
		if (n < 0 || sunxi_dphy_dt_read_config(&disp->dphy, n) != DRIVER_OK)
			return DRIVER_ERROR_INVALID;
		disp->dphy.clk = &disp->clk;
		disp->has_dphy = true;
		disp->if_cfg.phy_id = disp->dphy.id;
	}

	panel = sunxi_disp_dt_ref(node, "allwinner,panel");
	if (sunxi_panel_dt_read_config(&disp->panel, panel) != DRIVER_OK)
		return DRIVER_ERROR_INVALID;
	if (disp->iface == SUNXI_DISP_IF_DSI) {
		disp->if_cfg.dsi_lanes = disp->panel.dsi_lanes;
		disp->if_cfg.dsi_command_mode = !(disp->panel.dsi_mode_flags & SUNXI_DSI_MODE_VIDEO);
	}

	n = sunxi_disp_dt_ref(node, "allwinner,backlight");
	if (n >= 0 && sunxi_backlight_dt_read_config(&disp->backlight, n) != DRIVER_OK)
		return DRIVER_ERROR_INVALID;

	SYTERKIT_DT_TRACE_NODE("display", node);
	return DRIVER_OK;
}

static inline __attribute__((always_inline)) int sunxi_display_dt_read_alias(sunxi_display_t *disp, const char *alias)
{
	int node;

	if (alias == NULL)
		return DRIVER_ERROR_INVALID;
	node = syterkit_dt_alias_node(alias, SUNXI_DISPLAY_COMPATIBLE);
	if (node < 0)
		return DRIVER_ERROR_INVALID;
	return sunxi_display_dt_read_config(disp, node);
}

#endif
