/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Display pipeline: DE (UI layer passthrough) -> TCON -> RGB / LVDS / DSI -> panel.
 *
 * Everything that differs between SoCs is selected at run time from the
 * compatible strings of the device tree nodes (struct sunxi_*_variant); the
 * addresses, clocks, resets, pins and the panel come from the nodes
 * themselves.
 */
#ifndef __SUNXI_DISPLAY_H__
#define __SUNXI_DISPLAY_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <driver.h>
#include <drivers/gpio/gpio.h>
#include <drivers/pwm/pwm.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------ */
/* Common resources                                                    */
/* ------------------------------------------------------------------ */

/* A bit in a register (absolute address); reg == 0: not present. */
typedef struct {
	uintptr_t reg;
	uint8_t bit;
} sunxi_disp_bit_t;

/*
 * Register window plus the clock tree handles of one block, as described by
 * the node: reg, allwinner,clock-gate, allwinner,reset and
 * allwinner,module-clock = <reg mux>.
 *
 * The module clock register has its gate in bit 31 and the parent select in
 * bits 26:24; the divider layout and the meaning of the mux values are
 * block / SoC specific and kept in the variants.
 */
typedef struct {
	uintptr_t base;
	sunxi_disp_bit_t gate;	/* bus clock gate */
	sunxi_disp_bit_t reset;	/* bus reset (deasserted = 1) */
	uintptr_t mod_reg;	/* module clock register, 0: none */
	uint8_t mod_mux;	/* parent select written to bits 26:24 */
	uint8_t mod_set;	/* 1: the node asked for this mux */
	uint32_t mod_rate;	/* requested module clock, Hz, 0: unspecified */
	uint8_t mod_div;	/* fixed divider (>= 1): parent and PLL stay as they are, 0: not used */
} sunxi_disp_res_t;

/* Clock sources a module clock can pick (meaning of the mux in the variants). */
typedef enum {
	SUNXI_DISP_SRC_NONE = 0,
	SUNXI_DISP_SRC_HOSC,
	SUNXI_DISP_SRC_PLL_PERI_1X,
	SUNXI_DISP_SRC_PLL_PERI_2X,
	SUNXI_DISP_SRC_PLL_VIDEO_1X,
	SUNXI_DISP_SRC_PLL_VIDEO_4X,
} sunxi_disp_src_t;

#define SUNXI_DISP_MAX_PARENTS 4

/* Divider layout of a module clock register. */
typedef enum {
	SUNXI_DISP_DIV_LINEAR = 0,	/* rate = parent / (div_field + 1), field width given */
	SUNXI_DISP_DIV_MP,		/* P bits 9:8 (power of two) and M bits 3:0 (linear) */
} sunxi_disp_div_t;

/* One module clock: which sources the mux has and how it divides. */
typedef struct {
	sunxi_disp_div_t div;
	uint8_t div_width;			/* SUNXI_DISP_DIV_LINEAR: bits at 0 */
	sunxi_disp_src_t parents[SUNXI_DISP_MAX_PARENTS];
} sunxi_disp_modclk_t;

/*
 * SoC level clock and glue description (compatible of the display node).
 * Rates of the shared PLLs and PLL_VIDEO programming are chip specific.
 */
struct sunxi_disp_soc {
	const char *compatible;
	uint32_t hosc_hz;
	/* PLL_VIDEO register layout (offsets relative to the register in the node) */
	uint8_t pll_video_n_shift;
	uint8_t pll_video_n_width;
	uint8_t pll_video_n_bias;		/* field + bias = N */
	uint8_t pll_video_m_bit;		/* bit that divides the reference by two, 0xff: none */
	uint8_t pll_video_en_bit;
	uint8_t pll_video_ldo_bit;		/* 0xff: none */
	uint8_t pll_video_lock_en_bit;		/* 0xff: none */
	uint8_t pll_video_locked_bit;		/* 0xff: none */
	uint8_t pll_video_out_bit;		/* output gate, 0xff: none */
	uint32_t pll_video_min_hz;
	uint32_t pll_video_max_hz;
	uint8_t pll_video_4x_div;		/* VIDEO_4X = pll, VIDEO_1X = pll / this */
	/* PLL_PERI: rate = hosc / (m? 2:1) / p0 * n, 1X = 2X / 2 */
	uint8_t pll_peri_n_shift;
	uint8_t pll_peri_n_width;
	uint8_t pll_peri_n_bias;
	uint8_t pll_peri_p0_shift;
	uint8_t pll_peri_p0_width;
	uint8_t pll_peri_m_bit;			/* reference / 2, 0xff: none */
	/* write-once bring-up of glue registers (system control etc.), may be NULL */
	void (*soc_init)(void);
	/* D-PHY termination trim copied from the fuses, may be NULL */
	void (*dphy_calibrate)(void);
};

/* PLL helpers (display-clk.c), all take the PLL registers from the display node. */
typedef struct {
	const struct sunxi_disp_soc *soc;
	uintptr_t pll_video_reg;
	uintptr_t pll_peri_reg;
} sunxi_disp_clk_t;

uint32_t sunxi_disp_src_rate(const sunxi_disp_clk_t *clk, sunxi_disp_src_t src);
int sunxi_disp_pll_video_set(const sunxi_disp_clk_t *clk, uint32_t hz);
void sunxi_disp_pll_video_enable(const sunxi_disp_clk_t *clk, bool on);
/* bus gate + reset of a block (idempotent per call, no refcount) */
void sunxi_disp_res_bus_enable(const sunxi_disp_res_t *res, bool on);
/*
 * Program and gate the module clock. @rate_hz: wanted module clock rate; the
 * parent is mux res->mod_mux. If that parent is a PLL_VIDEO source, the PLL is
 * retuned to the wanted rate times @pll_mult (the block owns it then).
 * Returns the rate that was set (0 on error).
 */
uint32_t sunxi_disp_modclk_set(const sunxi_disp_clk_t *clk, const sunxi_disp_res_t *res,
			       const sunxi_disp_modclk_t *desc, uint32_t rate_hz);
void sunxi_disp_modclk_gate(const sunxi_disp_res_t *res, bool on);
uint32_t sunxi_disp_modclk_get(const sunxi_disp_clk_t *clk, const sunxi_disp_res_t *res,
			       const sunxi_disp_modclk_t *desc);

/* ------------------------------------------------------------------ */
/* Timing / panel                                                      */
/* ------------------------------------------------------------------ */
typedef struct {
	uint32_t pixel_clock_hz;
	uint16_t hactive, hfront_porch, hback_porch, hsync_len;
	uint16_t vactive, vfront_porch, vback_porch, vsync_len;
	uint8_t hsync_active;	/* 1: active high */
	uint8_t vsync_active;
	uint8_t de_active;
	uint8_t pixelclk_active;	/* 1: data valid on rising edge */
	/* derived */
	uint16_t htotal, vtotal;
} sunxi_disp_timing_t;

typedef enum {
	SUNXI_DISP_IF_RGB = 0,
	SUNXI_DISP_IF_LVDS,
	SUNXI_DISP_IF_DSI,
} sunxi_disp_if_t;

/* Bus format of the panel side, drives TCON dithering and output width. */
typedef enum {
	SUNXI_DISP_BUS_RGB888 = 0,
	SUNXI_DISP_BUS_RGB666,
	SUNXI_DISP_BUS_RGB565,
} sunxi_disp_bus_t;

/* Command sequence (cells, see dt-bindings/display/sunxi-display.h). */
typedef struct {
	const uint32_t *cells;	/* big endian device tree cells */
	uint32_t count;		/* number of cells */
} sunxi_disp_seq_t;

#define SUNXI_DSI_MODE_VIDEO		(1U << 0)
#define SUNXI_DSI_MODE_VIDEO_BURST	(1U << 1)
#define SUNXI_DSI_MODE_VIDEO_SYNC_PULSE	(1U << 2)
#define SUNXI_DSI_MODE_LPM		(1U << 3)
#define SUNXI_DSI_CLOCK_NON_CONTINUOUS	(1U << 4)
#define SUNXI_DSI_MODE_NO_EOT_PACKET	(1U << 5)

typedef enum {
	SUNXI_DSI_FMT_RGB888 = 0,
	SUNXI_DSI_FMT_RGB666,		/* 18 bit, loosely packed */
	SUNXI_DSI_FMT_RGB666_PACKED,
	SUNXI_DSI_FMT_RGB565,
} sunxi_dsi_format_t;

typedef struct {
	sunxi_disp_timing_t timing;
	sunxi_disp_bus_t bus;
	uint8_t dsi_lanes;
	uint8_t dsi_channel;
	sunxi_dsi_format_t dsi_format;
	uint32_t dsi_mode_flags;
	sunxi_disp_seq_t power_on, power_off, init, exit;
	uint32_t enable_delay_ms;
	uint16_t width_mm, height_mm;
	/* GPIO controller the SUNXI_CMD_GPIO pin numbers (bank * 32 + n) refer to */
	uintptr_t gpio_base;
	uint8_t gpio_bank_base;
} sunxi_panel_t;

/* Executes a command sequence; @dsi may be NULL when no DSI command appears. */
struct sunxi_dsi;
int sunxi_panel_run_seq(const sunxi_panel_t *panel, const sunxi_disp_seq_t *seq, struct sunxi_dsi *dsi);

/* ------------------------------------------------------------------ */
/* Combo D-PHY (DSI / LVDS)                                            */
/* ------------------------------------------------------------------ */
typedef struct sunxi_dphy {
	sunxi_disp_res_t res;
	const struct sunxi_dphy_variant *var;
	const sunxi_disp_clk_t *clk;
	uint8_t id;
	uint32_t hs_clk_hz;
	bool powered;
} sunxi_dphy_t;

typedef struct {
	uint32_t lanes;
	uint32_t pixclk_hz;
	uint32_t bpp;
	uint8_t hs_trail;	/* 0: default */
	uint8_t clk_trail;	/* 0: default */
} sunxi_dphy_dsi_cfg_t;

int sunxi_dphy_power_on(sunxi_dphy_t *phy);
void sunxi_dphy_power_off(sunxi_dphy_t *phy);
int sunxi_dphy_lvds_enable(sunxi_dphy_t *phy, bool dual_link);
void sunxi_dphy_lvds_disable(sunxi_dphy_t *phy);
int sunxi_dphy_dsi_enable(sunxi_dphy_t *phy, const sunxi_dphy_dsi_cfg_t *cfg, uint32_t *hs_clk_hz);
void sunxi_dphy_dsi_disable(sunxi_dphy_t *phy);
void sunxi_dphy_dump(sunxi_dphy_t *phy);

/* ------------------------------------------------------------------ */
/* DSI host                                                            */
/* ------------------------------------------------------------------ */
typedef struct sunxi_dsi {
	sunxi_disp_res_t res;
	const struct sunxi_dsi_variant *var;
	const sunxi_disp_clk_t *clk;
	sunxi_dphy_t *phy;
	uint8_t id;
	uint32_t mod_hz;
	bool prepared;
} sunxi_dsi_t;

typedef struct {
	const sunxi_panel_t *panel;
	sunxi_disp_timing_t timing;
} sunxi_dsi_cfg_t;

/* Power the host and the phy, program the video timing and the link. Lanes stay in LP-11. */
int sunxi_dsi_prepare(sunxi_dsi_t *dsi, const sunxi_panel_t *panel);
/* Start the HS clock / video stream (after the TCON runs). */
int sunxi_dsi_start_video(sunxi_dsi_t *dsi);
void sunxi_dsi_stop_video(sunxi_dsi_t *dsi);
void sunxi_dsi_unprepare(sunxi_dsi_t *dsi);
/* Low level messages (panel commands). Returns bytes read or 0, or < 0 on error. */
int sunxi_dsi_dcs_write(sunxi_dsi_t *dsi, uint8_t cmd, const uint8_t *data, size_t len);
int sunxi_dsi_generic_write(sunxi_dsi_t *dsi, const uint8_t *data, size_t len);
void sunxi_dsi_dump(sunxi_dsi_t *dsi);
/* Command mode: start the DSI side of one frame; false when the link is busy. */
bool sunxi_dsi_frame_start(sunxi_dsi_t *dsi);

/* ------------------------------------------------------------------ */
/* TCON                                                                */
/* ------------------------------------------------------------------ */
typedef enum {
	SUNXI_TCON_PATTERN_NONE = 0,	/* DE pixels */
	SUNXI_TCON_PATTERN_COLORBAR,
	SUNXI_TCON_PATTERN_GRAYSCALE,
	SUNXI_TCON_PATTERN_BLACK_WHITE,
	SUNXI_TCON_PATTERN_BLACK,
	SUNXI_TCON_PATTERN_WHITE,
	SUNXI_TCON_PATTERN_GRID,
} sunxi_tcon_pattern_t;

struct sunxi_tcon_variant;
struct sunxi_tcon_top_variant;

typedef struct {
	sunxi_disp_res_t res;
	const struct sunxi_tcon_top_variant *var;
	uint32_t users;
} sunxi_tcon_top_t;

typedef struct {
	/* rgb */
	uint8_t hv_mode;	/* 0 parallel RGB, 8 serial RGB, 0xa serial + dummy, 0xc serial YUV, 0xe CCIR656 */
	uint8_t srgb_seq, syuv_seq, syuv_fdly;
	uint8_t rgb_swap, rb_swap, clk_phase;
	uint32_t io_adjust;
	/* lvds */
	bool lvds_dual_link, lvds_jeida;
	/* dsi (filled in by the display from the dsi node) */
	uint8_t dsi_id, phy_id, dsi_lanes;
	bool dsi_command_mode;
} sunxi_tcon_if_cfg_t;

typedef struct {
	sunxi_disp_res_t res;
	const struct sunxi_tcon_variant *var;
	const sunxi_disp_clk_t *clk;
	sunxi_tcon_top_t *top;	/* NULL when the SoC has none */
	uint8_t id;
	uint8_t de_port;
	sunxi_disp_if_t iface;
	sunxi_disp_timing_t timing;
	sunxi_tcon_if_cfg_t cfg;
	sunxi_disp_bus_t bus;
	uint32_t mod_hz, pixclk_hz, div;
	uint32_t start_delay;
	bool powered;
} sunxi_tcon_t;

int sunxi_tcon_prepare(sunxi_tcon_t *tcon, sunxi_disp_if_t iface, const sunxi_disp_timing_t *t,
		       sunxi_disp_bus_t bus, const sunxi_tcon_if_cfg_t *cfg);
int sunxi_tcon_enable(sunxi_tcon_t *tcon);
void sunxi_tcon_disable(sunxi_tcon_t *tcon);
void sunxi_tcon_unprepare(sunxi_tcon_t *tcon);
int sunxi_tcon_set_pattern(sunxi_tcon_t *tcon, sunxi_tcon_pattern_t pattern);
bool sunxi_tcon_check_underflow(sunxi_tcon_t *tcon);
uint32_t sunxi_tcon_get_line(sunxi_tcon_t *tcon);
void sunxi_tcon_dump(sunxi_tcon_t *tcon);
/* Command mode: true (and the flag is cleared) when the frame counter expired, and start the TCON side of the frame. */
bool sunxi_tcon_frame_flag(sunxi_tcon_t *tcon);
void sunxi_tcon_trigger(sunxi_tcon_t *tcon);

/* ------------------------------------------------------------------ */
/* DE: one UI channel straight into the TCON                           */
/* ------------------------------------------------------------------ */
typedef enum {
	SUNXI_DE_FMT_ARGB8888 = 0,
	SUNXI_DE_FMT_XRGB8888,
	SUNXI_DE_FMT_RGB888,
	SUNXI_DE_FMT_RGB565,
} sunxi_de_format_t;

struct sunxi_de_variant;

typedef struct {
	sunxi_disp_res_t res;
	const struct sunxi_de_variant *var;
	const sunxi_disp_clk_t *clk;
	uint8_t port;		/* output port feeding the TCON */
	uint32_t width, height;
	bool powered;
	bool enabled;
} sunxi_de_t;

/* Bring up bus/clock and the pipeline for @width x @height output (layer stays off). */
int sunxi_de_init(sunxi_de_t *de, uint32_t width, uint32_t height);
/* Point the UI layer at a frame buffer (physical address) and commit. */
int sunxi_de_ui_set_fb(sunxi_de_t *de, uintptr_t addr, uint32_t width, uint32_t height,
		       uint32_t stride_bytes, sunxi_de_format_t fmt);
int sunxi_de_enable(sunxi_de_t *de, bool on);
void sunxi_de_deinit(sunxi_de_t *de);
void sunxi_de_dump(sunxi_de_t *de);

/* ------------------------------------------------------------------ */
/* Backlight                                                           */
/* ------------------------------------------------------------------ */
typedef enum {
	SUNXI_BACKLIGHT_NONE = 0,
	SUNXI_BACKLIGHT_GPIO,
	SUNXI_BACKLIGHT_PWM,
} sunxi_backlight_type_t;

typedef struct {
	sunxi_backlight_type_t type;
	gpio_mux_t gpio;		/* SUNXI_BACKLIGHT_GPIO: the enable pin */
	uint8_t gpio_active_high;
	sunxi_pwm_t pwm;		/* SUNXI_BACKLIGHT_PWM */
	uint8_t pwm_channel;
	uint32_t pwm_period_ns;
	uint8_t pwm_active_high;
	uint8_t level;			/* percent when switched on */
} sunxi_backlight_t;

int sunxi_backlight_set(sunxi_backlight_t *bl, bool on);

/* ------------------------------------------------------------------ */
/* The whole pipeline                                                  */
/* ------------------------------------------------------------------ */
typedef struct {
	int dt_node;
	sunxi_disp_clk_t clk;
	sunxi_disp_if_t iface;
	sunxi_panel_t panel;
	sunxi_tcon_top_t top;
	bool has_top;
	sunxi_tcon_t tcon;
	sunxi_de_t de;
	sunxi_dsi_t dsi;
	sunxi_dphy_t dphy;
	sunxi_tcon_if_cfg_t if_cfg;
	sunxi_backlight_t backlight;
	/* pads of the selected output (pinctrl-0 of the output node) */
#define SUNXI_DISP_MAX_PINS 32
	gpio_mux_t pins[SUNXI_DISP_MAX_PINS];
	uint8_t pin_count;
	int8_t pin_drive;		/* -1: leave the drive strength alone */
	sunxi_disp_bit_t lvds_reset;	/* LVDS bus reset, reg 0: none */
	bool has_dphy;
	/* frame buffer */
	uintptr_t fb_addr;
	uint32_t fb_width, fb_height, fb_stride;
	sunxi_de_format_t fb_format;
	bool initialized;
	bool running;
} sunxi_display_t;

int sunxi_display_init(sunxi_display_t *disp);
int sunxi_display_set_fb(sunxi_display_t *disp, uintptr_t addr, uint32_t width, uint32_t height,
			 uint32_t stride_bytes, sunxi_de_format_t fmt);
int sunxi_display_enable(sunxi_display_t *disp);
void sunxi_display_disable(sunxi_display_t *disp);
int sunxi_display_set_pattern(sunxi_display_t *disp, sunxi_tcon_pattern_t pattern);
void sunxi_display_dump(sunxi_display_t *disp);
/*
 * DSI command mode has no interrupt here: call this regularly (at least once per frame) to push the next frame to
 * the panel. Returns 1 when a frame was started, 0 otherwise (also in every other mode).
 */
int sunxi_display_poll(sunxi_display_t *disp);

#ifdef __cplusplus
}
#endif

#endif /* __SUNXI_DISPLAY_H__ */
