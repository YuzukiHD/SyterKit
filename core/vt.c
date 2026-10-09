/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file vt.c
 * @brief Text console with ANSI colors on a XRGB8888 frame buffer.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <cache.h>

#include <vt.h>
#include <vt_font.h>

#define VT_MAX_PARAMS 8
/* flush the cache after this many characters even without a newline */
#define VT_FLUSH_CHARS 256

#define SEL_DEFAULT (-1)
#define SEL_RGB	    0x1000000 /* 0x1000000 | 0xRRGGBB */

#define DEFAULT_FG 0xd0d0d0U
#define DEFAULT_BG 0x000000U
/* "black" text (ESC[30m, the trace log level) would vanish on a black background */
#define BLACK_FG   0x6e6e6eU

enum vt_state {
	ST_NORMAL,
	ST_ESC,
	ST_CSI,
};

static const uint32_t ansi_colors[16] = {
	0x000000, 0xcd3131, 0x0dbc79, 0xe5e510, 0x2472c8, 0xbc3fbc, 0x11a8cd, 0xe5e5e5,
	0x666666, 0xf14c4c, 0x23d18b, 0xf5f543, 0x3b8eea, 0xd670d6, 0x29b8db, 0xffffff,
};

#ifndef CONFIG_VT_EARLY_BUFFER_SIZE
#define CONFIG_VT_EARLY_BUFFER_SIZE 4096
#endif

/*
 * Characters written before vt_init(); replayed onto the screen by it. The static buffer sits in its own section
 * behind bss and stack (see the linker script): on SoCs whose display engine takes the top of the SRAM away, an
 * application has to move the text elsewhere with vt_set_memory() before it initializes the display.
 */
static char early_static[CONFIG_VT_EARLY_BUFFER_SIZE] __attribute__((section(".vt_early")));
static char *early_buf = early_static;
static uint32_t early_cap = sizeof(early_static);
static uint32_t early_len;
static bool vt_started;	/* vt_init() ran once: stop buffering */
static bool replaying;	/* no origin callbacks while the early text is drawn */

static struct {
	bool ready;
	vt_config_t cfg;
	uint32_t cols, rows;
	uint32_t cell_w, cell_h;
	uint32_t total_height;
	uint32_t y0;		/* first shown pixel row in the buffer */
	uint32_t col, row;
	uint32_t dirty_lo, dirty_hi;	/* pixel rows [lo, hi) not written back yet */
	uint32_t pending;
	uint32_t default_fg, default_bg;
	int fg, bg;
	bool bold, reverse;
	enum vt_state state;
	int params[VT_MAX_PARAMS];
	uint8_t nparams;
	bool have_param;
} vt;

static inline uint32_t *pixel_row(uint32_t y)
{
	return (uint32_t *)(vt.cfg.fb + (uintptr_t)y * vt.cfg.stride);
}

static void mark_dirty(uint32_t lo, uint32_t hi)
{
	if (vt.dirty_lo >= vt.dirty_hi) {
		vt.dirty_lo = lo;
		vt.dirty_hi = hi;
		return;
	}
	if (lo < vt.dirty_lo)
		vt.dirty_lo = lo;
	if (hi > vt.dirty_hi)
		vt.dirty_hi = hi;
}

void vt_flush(void)
{
	if (!vt.ready)
		return;
	if (vt.dirty_lo < vt.dirty_hi)
		flush_dcache_range(vt.cfg.fb + (uintptr_t)vt.dirty_lo * vt.cfg.stride,
				   vt.cfg.fb + (uintptr_t)vt.dirty_hi * vt.cfg.stride);
	vt.dirty_lo = vt.dirty_hi = 0;
	vt.pending = 0;
}

static uint32_t palette256(int n)
{
	if (n < 16)
		return ansi_colors[n];
	if (n < 232) {
		static const uint8_t lv[6] = { 0, 95, 135, 175, 215, 255 };

		n -= 16;
		return ((uint32_t)lv[n / 36] << 16) | ((uint32_t)lv[(n / 6) % 6] << 8) | lv[n % 6];
	}
	n = 8 + (n - 232) * 10;
	return ((uint32_t)n << 16) | ((uint32_t)n << 8) | (uint32_t)n;
}

static uint32_t resolve_fg(void)
{
	if (vt.fg == SEL_DEFAULT)
		return vt.bold ? 0xffffffU : vt.default_fg;
	if (vt.fg & SEL_RGB)
		return (uint32_t)vt.fg & 0xffffffU;
	if (vt.fg == 0 && !vt.bold)
		return BLACK_FG;
	if (vt.bold && vt.fg < 8)
		return ansi_colors[vt.fg + 8];
	return palette256(vt.fg);
}

static uint32_t resolve_bg(void)
{
	if (vt.bg == SEL_DEFAULT)
		return vt.default_bg;
	if (vt.bg & SEL_RGB)
		return (uint32_t)vt.bg & 0xffffffU;
	return palette256(vt.bg);
}

static void fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color)
{
	uint32_t i, j;

	for (j = 0; j < h; j++) {
		uint32_t *p = pixel_row(y + j) + x;

		for (i = 0; i < w; i++)
			p[i] = color;
	}
	mark_dirty(y, y + h);
}

static void clear_cells(uint32_t row, uint32_t col, uint32_t count)
{
	if (count == 0)
		return;
	fill_rect(col * vt.cell_w, vt.y0 + row * vt.cell_h, count * vt.cell_w, vt.cell_h, resolve_bg());
}

static void draw_glyph(uint32_t row, uint32_t col, unsigned char ch)
{
	uint32_t fg = resolve_fg(), bg = resolve_bg();
	const unsigned char *g;
	uint32_t x = col * vt.cell_w, y = vt.y0 + row * vt.cell_h;
	uint32_t s = vt.cfg.scale, gy, gx, sy, sx;

	if (vt.reverse) {
		uint32_t t = fg;

		fg = bg;
		bg = t;
	}
	if (ch < VT_FONT_FIRST || ch > VT_FONT_LAST)
		ch = '?';
	g = &vt_font_8x16[(ch - VT_FONT_FIRST) * VT_FONT_HEIGHT];

	for (gy = 0; gy < VT_FONT_HEIGHT; gy++) {
		unsigned char bits = g[gy];

		for (sy = 0; sy < s; sy++) {
			uint32_t *p = pixel_row(y + gy * s + sy) + x;

			for (gx = 0; gx < VT_FONT_WIDTH; gx++) {
				uint32_t c = (bits & (0x80U >> gx)) ? fg : bg;

				for (sx = 0; sx < s; sx++)
					*p++ = c;
			}
		}
	}
	mark_dirty(y, y + vt.cell_h);
}

/* Word wise forward copy; the destination is always below the source. */
static void copy_up(uint32_t dst_y, uint32_t src_y, uint32_t lines)
{
	uint32_t *d = pixel_row(dst_y), *s = pixel_row(src_y);
	uint32_t words = lines * vt.cfg.stride / 4U, i;

	for (i = 0; i < words; i++)
		d[i] = s[i];
}

static void scroll_up(void)
{
	bool hw = vt.cfg.set_origin != NULL;

	if (hw && vt.y0 + vt.cell_h + vt.rows * vt.cell_h <= vt.total_height) {
		vt.y0 += vt.cell_h;
		clear_cells(vt.rows - 1, 0, vt.cols);
		vt_flush();
		if (!replaying)
			vt.cfg.set_origin(vt.cfg.ctx, vt.y0);
		return;
	}

	/* copy the kept rows to the top of the buffer (or one row up without a movable origin) */
	copy_up(0, vt.y0 + vt.cell_h, (vt.rows - 1) * vt.cell_h);
	vt.y0 = 0;
	fill_rect(0, (vt.rows - 1) * vt.cell_h, vt.cfg.width, vt.total_height - (vt.rows - 1) * vt.cell_h,
		  vt.default_bg);
	mark_dirty(0, vt.total_height);
	vt_flush();
	if (hw && !replaying)
		vt.cfg.set_origin(vt.cfg.ctx, 0);
}

static void newline(void)
{
	vt.col = 0;
	if (vt.row + 1 >= vt.rows)
		scroll_up();
	else
		vt.row++;
}

void vt_clear(void)
{
	if (!vt.ready)
		return;
	fill_rect(0, 0, vt.cfg.width, vt.total_height, vt.default_bg);
	vt.y0 = 0;
	vt.col = vt.row = 0;
	vt_flush();
	if (vt.cfg.set_origin && !replaying)
		vt.cfg.set_origin(vt.cfg.ctx, 0);
}

static void reset_attrs(void)
{
	vt.fg = vt.bg = SEL_DEFAULT;
	vt.bold = vt.reverse = false;
}

static int param(unsigned int i, int def)
{
	if (i >= vt.nparams || vt.params[i] < 0)
		return def;
	return vt.params[i];
}

static void sgr(void)
{
	unsigned int i;

	if (vt.nparams == 0) {
		reset_attrs();
		return;
	}
	for (i = 0; i < vt.nparams; i++) {
		int p = vt.params[i] < 0 ? 0 : vt.params[i];

		if (p == 0) {
			reset_attrs();
		} else if (p == 1) {
			vt.bold = true;
		} else if (p == 7) {
			vt.reverse = true;
		} else if (p == 22) {
			vt.bold = false;
		} else if (p == 27) {
			vt.reverse = false;
		} else if (p >= 30 && p <= 37) {
			vt.fg = p - 30;
		} else if (p >= 40 && p <= 47) {
			vt.bg = p - 40;
		} else if (p >= 90 && p <= 97) {
			vt.fg = p - 90 + 8;
		} else if (p >= 100 && p <= 107) {
			vt.bg = p - 100 + 8;
		} else if (p == 39) {
			vt.fg = SEL_DEFAULT;
		} else if (p == 49) {
			vt.bg = SEL_DEFAULT;
		} else if (p == 38 || p == 48) {
			int sel = SEL_DEFAULT;

			if (i + 2 < vt.nparams && vt.params[i + 1] == 5) {
				int n = vt.params[i + 2];

				sel = (n < 0 ? 0 : n > 255 ? 255 : n);
				i += 2;
			} else if (i + 4 < vt.nparams && vt.params[i + 1] == 2) {
				uint32_t r = (uint32_t)vt.params[i + 2] & 0xffU, g = (uint32_t)vt.params[i + 3] & 0xffU,
					 b = (uint32_t)vt.params[i + 4] & 0xffU;

				sel = (int)(SEL_RGB | (r << 16) | (g << 8) | b);
				i += 4;
			} else {
				break;
			}
			if (p == 38)
				vt.fg = sel;
			else
				vt.bg = sel;
		}
	}
}

static void csi_dispatch(char final)
{
	int n;

	switch (final) {
	case 'm':
		sgr();
		break;
	case 'A':
		n = param(0, 1);
		vt.row = (uint32_t)n > vt.row ? 0 : vt.row - (uint32_t)n;
		break;
	case 'B':
		vt.row += (uint32_t)param(0, 1);
		if (vt.row >= vt.rows)
			vt.row = vt.rows - 1;
		break;
	case 'C':
		vt.col += (uint32_t)param(0, 1);
		if (vt.col >= vt.cols)
			vt.col = vt.cols - 1;
		break;
	case 'D':
		n = param(0, 1);
		vt.col = (uint32_t)n > vt.col ? 0 : vt.col - (uint32_t)n;
		break;
	case 'G':
		n = param(0, 1);
		vt.col = (uint32_t)(n < 1 ? 0 : n - 1);
		if (vt.col >= vt.cols)
			vt.col = vt.cols - 1;
		break;
	case 'H':
	case 'f':
		n = param(0, 1);
		vt.row = (uint32_t)(n < 1 ? 0 : n - 1);
		if (vt.row >= vt.rows)
			vt.row = vt.rows - 1;
		n = param(1, 1);
		vt.col = (uint32_t)(n < 1 ? 0 : n - 1);
		if (vt.col >= vt.cols)
			vt.col = vt.cols - 1;
		break;
	case 'J':
		n = param(0, 0);
		if (n == 2 || n == 3) {
			uint32_t r;

			for (r = 0; r < vt.rows; r++)
				clear_cells(r, 0, vt.cols);
		} else if (n == 1) {
			uint32_t r;

			for (r = 0; r < vt.row; r++)
				clear_cells(r, 0, vt.cols);
			clear_cells(vt.row, 0, vt.col + 1);
		} else {
			uint32_t r;

			clear_cells(vt.row, vt.col, vt.cols - vt.col);
			for (r = vt.row + 1; r < vt.rows; r++)
				clear_cells(r, 0, vt.cols);
		}
		break;
	case 'K':
		n = param(0, 0);
		if (n == 2)
			clear_cells(vt.row, 0, vt.cols);
		else if (n == 1)
			clear_cells(vt.row, 0, vt.col + 1);
		else
			clear_cells(vt.row, vt.col, vt.cols - vt.col);
		break;
	default:
		break;
	}
}

static void put_printable(unsigned char c)
{
	if (vt.col >= vt.cols)
		newline();
	draw_glyph(vt.row, vt.col, c);
	vt.col++;
}

void vt_putc(char ch)
{
	unsigned char c = (unsigned char)ch;

	if (!vt.ready) {
		if (!vt_started && early_len < early_cap)
			early_buf[early_len++] = ch;
		return;
	}

	switch (vt.state) {
	case ST_ESC:
		if (c == '[') {
			vt.state = ST_CSI;
			vt.nparams = 0;
			vt.have_param = false;
			vt.params[0] = -1;
		} else {
			vt.state = ST_NORMAL;
		}
		return;
	case ST_CSI:
		if (c >= '0' && c <= '9') {
			if (vt.nparams < VT_MAX_PARAMS) {
				int *p = &vt.params[vt.nparams];

				if (!vt.have_param) {
					*p = 0;
					vt.have_param = true;
				}
				if (*p < 100000)
					*p = *p * 10 + (c - '0');
			}
		} else if (c == ';') {
			if (vt.nparams + 1 < VT_MAX_PARAMS) {
				if (!vt.have_param)
					vt.params[vt.nparams] = -1;
				vt.nparams++;
				vt.params[vt.nparams] = -1;
				vt.have_param = false;
			}
		} else if (c >= 0x40 && c <= 0x7e) {
			if (vt.have_param || vt.nparams > 0)
				vt.nparams++;
			csi_dispatch((char)c);
			vt.state = ST_NORMAL;
		} else if (c < 0x20 || c > 0x3f) {
			vt.state = ST_NORMAL;	/* malformed */
		}
		/* 0x20..0x3f other than digits and ';' (private markers like '?') are skipped */
		return;
	default:
		break;
	}

	switch (c) {
	case 0x1b:
		vt.state = ST_ESC;
		break;
	case '\n':
		newline();
		vt_flush();
		break;
	case '\r':
		vt.col = 0;
		break;
	case '\b':
		if (vt.col > 0)
			vt.col--;
		break;
	case '\t': {
		uint32_t next = (vt.col + 8U) & ~7U;

		while (vt.col < next && vt.col < vt.cols) {
			draw_glyph(vt.row, vt.col, ' ');
			vt.col++;
		}
		break;
	}
	default:
		if (c >= 0x80U && c < 0xc0U)
			break;		/* UTF-8 continuation byte of a sequence already shown */
		if (c < 0x20 || c == 0x7f)
			break;
		put_printable(c >= 0x80 ? '?' : c);
		if (++vt.pending >= VT_FLUSH_CHARS)
			vt_flush();
		break;
	}
}

void vt_write(const char *s, size_t len)
{
	while (len--)
		vt_putc(*s++);
	vt_flush();
}

void vt_set_memory(void *mem, size_t size)
{
	uint32_t i, n;

	if (mem == NULL || size == 0 || vt_started)
		return;
	n = early_len < size ? early_len : (uint32_t)size;
	for (i = 0; i < n; i++)
		((char *)mem)[i] = early_buf[i];
	early_buf = mem;
	early_cap = (uint32_t)size;
	early_len = n;
}

int vt_init(const vt_config_t *cfg)
{
	uint32_t scale;

	if (cfg == NULL || cfg->fb == 0 || (cfg->fb & 3U) || (cfg->stride & 3U))
		return -1;
	scale = cfg->scale ? cfg->scale : 1;
	if (cfg->width < VT_FONT_WIDTH * scale || cfg->height < VT_FONT_HEIGHT * scale ||
	    cfg->stride < cfg->width * 4U)
		return -1;

	vt.ready = false;
	vt.cfg = *cfg;
	vt.cfg.scale = (uint8_t)scale;
	vt.cell_w = VT_FONT_WIDTH * scale;
	vt.cell_h = VT_FONT_HEIGHT * scale;
	vt.cols = cfg->width / vt.cell_w;
	vt.rows = cfg->height / vt.cell_h;
	vt.total_height = cfg->total_height > cfg->height ? cfg->total_height : cfg->height;
	vt.default_fg = (cfg->fg || cfg->bg) ? cfg->fg : DEFAULT_FG;
	vt.default_bg = (cfg->fg || cfg->bg) ? cfg->bg : DEFAULT_BG;
	vt.y0 = vt.col = vt.row = 0;
	vt.dirty_lo = vt.dirty_hi = vt.pending = 0;
	vt.state = ST_NORMAL;
	reset_attrs();
	vt.ready = true;
	vt_started = true;
	replaying = true;
	vt_clear();
	{
		uint32_t i, n = early_len;

		early_len = 0;
		for (i = 0; i < n; i++)
			vt_putc(early_buf[i]);
	}
	replaying = false;
	vt_flush();
	return 0;
}

void vt_deinit(void)
{
	vt_started = true;
	vt_flush();
	vt.ready = false;
}

bool vt_is_ready(void)
{
	return vt.ready;
}

uint32_t vt_cols(void)
{
	return vt.ready ? vt.cols : 0;
}

uint32_t vt_rows(void)
{
	return vt.ready ? vt.rows : 0;
}

uint32_t vt_origin(void)
{
	return vt.ready ? vt.y0 : 0;
}
