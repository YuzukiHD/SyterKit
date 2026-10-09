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

static void display_apply_pins(sunxi_display_t *d)
{
	unsigned int i;

	for (i = 0; i < d->pin_count; i++) {
		sunxi_gpio_init(&d->pins[i]);
		if (d->pin_drive >= 0)
			sunxi_gpio_set_drv(&d->pins[i], (gpio_drv_t)d->pin_drive);
	}
}

int sunxi_display_init(sunxi_display_t *d)
{
	int ret;

	if (!d || !d->clk.soc || d->initialized)
		return DRIVER_ERROR_INVALID;
	if (d->clk.soc->soc_init)
		d->clk.soc->soc_init();

	d->fb_width = d->panel.timing.hactive;
	d->fb_height = d->panel.timing.vactive;
	ret = sunxi_de_init(&d->de, d->fb_width, d->fb_height);
	if (ret) {
		pr_err("DE init failed: %d\n", ret);
		return ret;
	}
	d->initialized = true;
	return DRIVER_OK;
}

int sunxi_display_set_fb(sunxi_display_t *d, uintptr_t addr, uint32_t width, uint32_t height, uint32_t stride_bytes,
	sunxi_de_format_t fmt)
{
	int ret;

	if (!d || !d->initialized)
		return DRIVER_ERROR_INVALID;
	ret = sunxi_de_ui_set_fb(&d->de, addr, width, height, stride_bytes, fmt);
	if (ret)
		return ret;
	d->fb_addr = addr;
	d->fb_width = width;
	d->fb_height = height;
	d->fb_stride = stride_bytes;
	d->fb_format = fmt;
	return DRIVER_OK;
}

static int display_enable_dsi(sunxi_display_t *d)
{
	int ret;

	ret = sunxi_tcon_prepare(&d->tcon, d->iface, &d->panel.timing, d->panel.bus, &d->if_cfg);
	if (ret)
		return ret;
	if (d->clk.soc->dphy_calibrate)
		d->clk.soc->dphy_calibrate();
	ret = sunxi_dsi_prepare(&d->dsi, &d->panel);
	if (ret) {
		sunxi_tcon_unprepare(&d->tcon);
		return ret;
	}
	/* panel power and register init over LP commands, the clock lane runs in HS */
	sunxi_panel_run_seq(&d->panel, &d->panel.power_on, &d->dsi);
	ret = sunxi_panel_run_seq(&d->panel, &d->panel.init, &d->dsi);
	if (ret)
		pr_err("panel init sequence failed: %d\n", ret);
	sunxi_tcon_enable(&d->tcon);
	sunxi_dsi_start_video(&d->dsi);
	return DRIVER_OK;
}

static int display_enable_lvds(sunxi_display_t *d)
{
	int ret;

	if (d->lvds_reset.reg)
		setbits_le32(d->lvds_reset.reg, 1U << d->lvds_reset.bit);
	ret = sunxi_tcon_prepare(&d->tcon, d->iface, &d->panel.timing, d->panel.bus, &d->if_cfg);
	if (ret)
		return ret;
	if (d->clk.soc->dphy_calibrate)
		d->clk.soc->dphy_calibrate();
	if (sunxi_dphy_power_on(&d->dphy) == 0)
		sunxi_dphy_lvds_enable(&d->dphy, d->if_cfg.lvds_dual_link);
	sunxi_panel_run_seq(&d->panel, &d->panel.power_on, NULL);
	sunxi_tcon_enable(&d->tcon);
	return DRIVER_OK;
}

static int display_enable_rgb(sunxi_display_t *d)
{
	int ret;

	ret = sunxi_tcon_prepare(&d->tcon, d->iface, &d->panel.timing, d->panel.bus, &d->if_cfg);
	if (ret)
		return ret;
	sunxi_panel_run_seq(&d->panel, &d->panel.power_on, NULL);
	sunxi_tcon_enable(&d->tcon);
	return DRIVER_OK;
}

int sunxi_display_enable(sunxi_display_t *d)
{
	int ret;

	if (!d || !d->initialized || d->running)
		return DRIVER_ERROR_INVALID;

	display_apply_pins(d);
	/* the DE feeds the TCON from the first frame on */
	if (d->fb_addr) {
		sunxi_de_enable(&d->de, true);
	}

	switch (d->iface) {
	case SUNXI_DISP_IF_RGB:
		ret = display_enable_rgb(d);
		break;
	case SUNXI_DISP_IF_LVDS:
		ret = display_enable_lvds(d);
		break;
	case SUNXI_DISP_IF_DSI:
		ret = display_enable_dsi(d);
		break;
	default:
		ret = DRIVER_ERROR_INVALID;
		break;
	}
	if (ret) {
		pr_err("enable failed: %d\n", ret);
		return ret;
	}

	if (d->panel.enable_delay_ms)
		mdelay(d->panel.enable_delay_ms);
	ret = sunxi_backlight_set(&d->backlight, true);
	if (ret)
		pr_warn("backlight: %d\n", ret);
	d->running = true;
	return DRIVER_OK;
}

void sunxi_display_disable(sunxi_display_t *d)
{
	if (!d || !d->running)
		return;
	sunxi_backlight_set(&d->backlight, false);
	sunxi_tcon_disable(&d->tcon);
	if (d->iface == SUNXI_DISP_IF_DSI) {
		sunxi_panel_run_seq(&d->panel, &d->panel.exit, &d->dsi);
		sunxi_dsi_stop_video(&d->dsi);
		sunxi_panel_run_seq(&d->panel, &d->panel.power_off, &d->dsi);
		sunxi_dsi_unprepare(&d->dsi);
	} else {
		if (d->iface == SUNXI_DISP_IF_LVDS) {
			sunxi_dphy_lvds_disable(&d->dphy);
			sunxi_dphy_power_off(&d->dphy);
		}
		sunxi_panel_run_seq(&d->panel, &d->panel.power_off, NULL);
	}
	sunxi_tcon_unprepare(&d->tcon);
	sunxi_de_enable(&d->de, false);
	d->running = false;
}

int sunxi_display_set_pattern(sunxi_display_t *d, sunxi_tcon_pattern_t pattern)
{
	return d ? sunxi_tcon_set_pattern(&d->tcon, pattern) : DRIVER_ERROR_INVALID;
}

void sunxi_display_dump(sunxi_display_t *d)
{
	if (!d)
		return;
	pr_debug("%s, %ux%u@%uHz pixclk, fb %p stride %u\n",
		d->iface == SUNXI_DISP_IF_RGB  ? "rgb" :
		d->iface == SUNXI_DISP_IF_LVDS ? "lvds" :
						 "dsi",
		(unsigned int)d->panel.timing.hactive, (unsigned int)d->panel.timing.vactive,
		(unsigned int)d->panel.timing.pixel_clock_hz, (void *)d->fb_addr, (unsigned int)d->fb_stride);
	sunxi_de_dump(&d->de);
	sunxi_tcon_dump(&d->tcon);
	if (d->iface == SUNXI_DISP_IF_DSI) {
		sunxi_dsi_dump(&d->dsi);
		sunxi_dphy_dump(&d->dphy);
	}
}

int sunxi_display_poll(sunxi_display_t *d)
{
	if (!d || !d->running || d->iface != SUNXI_DISP_IF_DSI || !d->if_cfg.dsi_command_mode)
		return 0;
	if (!sunxi_tcon_frame_flag(&d->tcon))
		return 0;
	/* the interface first, then the TCON */
	if (!sunxi_dsi_frame_start(&d->dsi))
		return 0;
	sunxi_tcon_trigger(&d->tcon);
	return 1;
}

DT2C_DRIVER_COMPAT("allwinner,sunxi-display");
DT2C_DRIVER_COMPAT("allwinner,sunxi-rgb");
DT2C_DRIVER_COMPAT("allwinner,sunxi-lvds");
DT2C_DRIVER_COMPAT("allwinner,sunxi-panel");
DT2C_DRIVER_COMPAT("allwinner,sunxi-backlight");
