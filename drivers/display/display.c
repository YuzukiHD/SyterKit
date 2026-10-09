/* SPDX-License-Identifier: GPL-2.0+ */
#define pr_fmt(fmt) "display: " fmt

/*
 * Display pipeline sequencing: DE (one UI layer straight through) -> TCON ->
 * RGB / LVDS / DSI -> panel + backlight.
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

#include <drivers/display/display.h>

extern const struct sunxi_disp_soc sunxi_disp_soc_sun252iw2;

static const struct sunxi_disp_soc *const disp_socs[] = {
	&sunxi_disp_soc_sun252iw2,
};

const struct sunxi_disp_soc *sunxi_disp_soc_find(const char *name)
{
	unsigned int i;

	for (i = 0; name != NULL && i < sizeof(disp_socs) / sizeof(disp_socs[0]); i++)
		if (strcmp(disp_socs[i]->compatible, name) == 0)
			return disp_socs[i];
	return NULL;
}

static void display_apply_pins(sunxi_display_t *display)
{
	unsigned int i;

	for (i = 0; i < display->pin_count; i++) {
		sunxi_gpio_init(&display->pins[i]);
		if (display->pin_drive >= 0)
			sunxi_gpio_set_drv(&display->pins[i], (gpio_drv_t)display->pin_drive);
	}
}

int sunxi_display_init(sunxi_display_t *display)
{
	int ret;

	if (!display || !display->clk.soc || display->initialized)
		return DRIVER_ERROR_INVALID;
	if (display->clk.soc->soc_init)
		display->clk.soc->soc_init();

	display->fb_width = display->panel.timing.hactive;
	display->fb_height = display->panel.timing.vactive;
	ret = sunxi_de_init(&display->de, display->fb_width, display->fb_height);
	if (ret) {
		pr_err("DE init failed: %d\n", ret);
		return ret;
	}
	display->initialized = true;
	return DRIVER_OK;
}

int sunxi_display_set_fb(sunxi_display_t *display, uintptr_t addr, uint32_t width, uint32_t height,
	uint32_t stride_bytes, sunxi_de_format_t fmt)
{
	int ret;

	if (!display || !display->initialized)
		return DRIVER_ERROR_INVALID;
	ret = sunxi_de_ui_set_fb(&display->de, addr, width, height, stride_bytes, fmt);
	if (ret)
		return ret;
	display->fb_addr = addr;
	display->fb_width = width;
	display->fb_height = height;
	display->fb_stride = stride_bytes;
	display->fb_format = fmt;
	return DRIVER_OK;
}

static int display_enable_dsi(sunxi_display_t *display)
{
	int ret;

	ret = sunxi_tcon_prepare(
		&display->tcon, display->iface, &display->panel.timing, display->panel.bus, &display->if_cfg);
	if (ret)
		return ret;
	if (display->clk.soc->dphy_calibrate)
		display->clk.soc->dphy_calibrate();
	ret = sunxi_dsi_prepare(&display->dsi, &display->panel);
	if (ret) {
		sunxi_tcon_unprepare(&display->tcon);
		return ret;
	}
	/* panel power and register init over LP commands, the clock lane runs in HS */
	sunxi_panel_run_seq(&display->panel, &display->panel.power_on, &display->dsi);
	ret = sunxi_panel_run_seq(&display->panel, &display->panel.init, &display->dsi);
	if (ret)
		pr_err("panel init sequence failed: %d\n", ret);
	sunxi_tcon_enable(&display->tcon);
	sunxi_dsi_start_video(&display->dsi);
	return DRIVER_OK;
}

static int display_enable_lvds(sunxi_display_t *display)
{
	int ret;

	if (display->lvds_reset.reg)
		setbits_le32(display->lvds_reset.reg, 1U << display->lvds_reset.bit);
	ret = sunxi_tcon_prepare(
		&display->tcon, display->iface, &display->panel.timing, display->panel.bus, &display->if_cfg);
	if (ret)
		return ret;
	if (display->clk.soc->dphy_calibrate)
		display->clk.soc->dphy_calibrate();
	if (sunxi_dphy_power_on(&display->dphy) == 0)
		sunxi_dphy_lvds_enable(&display->dphy, display->if_cfg.lvds_dual_link);
	sunxi_panel_run_seq(&display->panel, &display->panel.power_on, NULL);
	sunxi_tcon_enable(&display->tcon);
	return DRIVER_OK;
}

static int display_enable_rgb(sunxi_display_t *display)
{
	int ret;

	ret = sunxi_tcon_prepare(
		&display->tcon, display->iface, &display->panel.timing, display->panel.bus, &display->if_cfg);
	if (ret)
		return ret;
	sunxi_panel_run_seq(&display->panel, &display->panel.power_on, NULL);
	sunxi_tcon_enable(&display->tcon);
	return DRIVER_OK;
}

int sunxi_display_enable(sunxi_display_t *display)
{
	int ret;

	if (!display || !display->initialized || display->running)
		return DRIVER_ERROR_INVALID;

	display_apply_pins(display);
	/* the DE feeds the TCON from the first frame on */
	if (display->fb_addr) {
		sunxi_de_enable(&display->de, true);
	}

	switch (display->iface) {
	case SUNXI_DISP_IF_RGB:
		ret = display_enable_rgb(display);
		break;
	case SUNXI_DISP_IF_LVDS:
		ret = display_enable_lvds(display);
		break;
	case SUNXI_DISP_IF_DSI:
		ret = display_enable_dsi(display);
		break;
	default:
		ret = DRIVER_ERROR_INVALID;
		break;
	}
	if (ret) {
		pr_err("enable failed: %d\n", ret);
		return ret;
	}

	if (display->panel.enable_delay_ms)
		mdelay(display->panel.enable_delay_ms);
	ret = sunxi_backlight_set(&display->backlight, true);
	if (ret)
		pr_warn("backlight: %d\n", ret);
	display->running = true;
	return DRIVER_OK;
}

void sunxi_display_disable(sunxi_display_t *display)
{
	if (!display || !display->running)
		return;
	sunxi_backlight_set(&display->backlight, false);
	sunxi_tcon_disable(&display->tcon);
	if (display->iface == SUNXI_DISP_IF_DSI) {
		sunxi_panel_run_seq(&display->panel, &display->panel.exit, &display->dsi);
		sunxi_dsi_stop_video(&display->dsi);
		sunxi_panel_run_seq(&display->panel, &display->panel.power_off, &display->dsi);
		sunxi_dsi_unprepare(&display->dsi);
	} else {
		if (display->iface == SUNXI_DISP_IF_LVDS) {
			sunxi_dphy_lvds_disable(&display->dphy);
			sunxi_dphy_power_off(&display->dphy);
		}
		sunxi_panel_run_seq(&display->panel, &display->panel.power_off, NULL);
	}
	sunxi_tcon_unprepare(&display->tcon);
	sunxi_de_enable(&display->de, false);
	display->running = false;
}

int sunxi_display_set_pattern(sunxi_display_t *display, sunxi_tcon_pattern_t pattern)
{
	return display ? sunxi_tcon_set_pattern(&display->tcon, pattern) : DRIVER_ERROR_INVALID;
}

void sunxi_display_dump(sunxi_display_t *display)
{
	if (!display)
		return;
	pr_debug("%s, %ux%u@%uHz pixclk, fb %p stride %u\n",
		display->iface == SUNXI_DISP_IF_RGB  ? "rgb" :
		display->iface == SUNXI_DISP_IF_LVDS ? "lvds" :
						       "dsi",
		(unsigned int)display->panel.timing.hactive, (unsigned int)display->panel.timing.vactive,
		(unsigned int)display->panel.timing.pixel_clock_hz, (void *)display->fb_addr,
		(unsigned int)display->fb_stride);
	sunxi_de_dump(&display->de);
	sunxi_tcon_dump(&display->tcon);
	if (display->iface == SUNXI_DISP_IF_DSI) {
		sunxi_dsi_dump(&display->dsi);
		sunxi_dphy_dump(&display->dphy);
	}
}

int sunxi_display_poll(sunxi_display_t *display)
{
	if (!display || !display->running || display->iface != SUNXI_DISP_IF_DSI || !display->if_cfg.dsi_command_mode)
		return 0;
	if (!sunxi_tcon_frame_flag(&display->tcon))
		return 0;
	/* the interface first, then the TCON */
	if (!sunxi_dsi_frame_start(&display->dsi))
		return 0;
	sunxi_tcon_trigger(&display->tcon);
	return 1;
}

DT2C_DRIVER_COMPAT("allwinner,sunxi-display");
DT2C_DRIVER_COMPAT("allwinner,sunxi-rgb");
DT2C_DRIVER_COMPAT("allwinner,sunxi-lvds");
DT2C_DRIVER_COMPAT("allwinner,sunxi-panel");
DT2C_DRIVER_COMPAT("allwinner,sunxi-backlight");
