/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __VT_H__
#define __VT_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * @file vt.h
 * @brief Text console on a XRGB8888 frame buffer.
 *
 * The console draws an 8x16 font into a frame buffer and understands the usual
 * ANSI escape sequences, so the colored log lines of printk() appear in color
 * on the screen: SGR (16 colors, 256 colors, 24 bit colors, bold, reverse),
 * cursor movement (CUU/CUD/CUF/CUB/CUP), erase in display and in line.
 * `\n` also returns the cursor to the first column, `\r` only does that.
 *
 * Characters written before vt_init() are kept (CONFIG_VT_EARLY_BUFFER_SIZE) and drawn by vt_init().
 * With CONFIG_VT every character that goes to the log UART is also drawn here
 * once vt_init() succeeded.
 */

/**
 * @brief Console configuration.
 *
 * The frame buffer holds @p total_height pixel rows of which @p height are
 * shown. When @p set_origin is given and total_height > height, scrolling moves
 * the shown window down the buffer (one text row per scroll, no copying) and
 * only copies the content back to the top when the end of the buffer is reached.
 * Without it the content is copied up on every scroll.
 */
typedef struct {
	uintptr_t fb;		/* frame buffer address, 4 byte pixels, must be cacheable-flushable */
	uint32_t width;		/* shown size in pixels */
	uint32_t height;
	uint32_t stride;	/* bytes per pixel row */
	uint32_t total_height;	/* pixel rows in the buffer, 0: same as height */
	uint8_t scale;		/* integer font scale, 0: 1 */
	uint32_t fg;		/* default colors 0xRRGGBB, both 0: light gray on black */
	uint32_t bg;
	/** Called after the shown window moved; @p y_offset is the first pixel row to show. */
	void (*set_origin)(void *ctx, uint32_t y_offset);
	void *ctx;
} vt_config_t;

/**
 * @brief Move the text kept before vt_init() into @p mem (and keep collecting there).
 *
 * Call it once memory that stays valid is available, and before anything that can make the static buffer
 * unreadable (on the F101 the display initialization takes the top of the SRAM away). Has no effect after vt_init().
 */
void vt_set_memory(void *mem, size_t size);

/** @brief Attach the console to a frame buffer and clear it. Returns 0 on success. */
int vt_init(const vt_config_t *cfg);

/** @brief Stop drawing (the frame buffer stays as it is). */
void vt_deinit(void);

/** @brief True after vt_init(). */
bool vt_is_ready(void);

/** @brief Draw one character (escape sequences are parsed across calls). */
void vt_putc(char c);

/** @brief Draw @p len characters. */
void vt_write(const char *s, size_t len);

/** @brief Clear the screen and put the cursor at the top left. */
void vt_clear(void);

/** @brief Write the pixels drawn so far back to memory so the display engine sees them. */
void vt_flush(void);

/**
 * @brief First pixel row of the shown window. The early text replayed by vt_init() can already have scrolled,
 * so the first frame buffer address given to the display is fb + vt_origin() * stride.
 */
uint32_t vt_origin(void);

/** @brief Number of text columns and rows (0 before vt_init()). */
uint32_t vt_cols(void);
uint32_t vt_rows(void);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* __VT_H__ */
