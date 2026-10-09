/* SPDX-License-Identifier: GPL-2.0+ */

#include <drivers/serial/serial.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <cache.h>
#include <common.h>
#include <log.h>
#include <timer.h>

#include <dt-bindings/soc/sun252iw2.h>
#include <drivers/clk/clk.h>
#include <drivers/display/display.h>
#include <drivers/psram/psram.h>
#include <dt-compatible/display-dt.h>
#include <dt-compatible/psram-dt.h>

#define FB_BASE (SUNXI_PSRAM_BASE + 0x00800000U)

/* eight vertical colour bars with a horizontal brightness ramp */
static void draw_test_picture(uint32_t *fb, uint32_t w, uint32_t h, uint32_t stride_px)
{
	static const uint32_t bars[8] = { 0xffffff, 0xffff00, 0x00ffff, 0x00ff00,
					  0xff00ff, 0xff0000, 0x0000ff, 0x000000 };
	uint32_t x, y;

	for (y = 0; y < h; y++) {
		for (x = 0; x < w; x++) {
			uint32_t c = bars[x * 8U / w];
			uint32_t k = 255U - y * 200U / h;

			fb[y * stride_px + x] = (((c >> 16 & 0xff) * k / 255U) << 16) |
						(((c >> 8 & 0xff) * k / 255U) << 8) | ((c & 0xff) * k / 255U);
		}
	}
}

/*
 * The display engine takes the SRAM above 0x2b000 away from the CPU (see platform/sun252iw2/soc.c): the image, its
 * bss and the stack have to stay below that, so the display state and the stack live in the PSRAM.
 */
#define PSRAM_STACK_TOP (SUNXI_PSRAM_BASE + 0x00400000U)
#define DISP_STATE	(SUNXI_PSRAM_BASE + 0x00300000U)

static void call_on_stack(void (*fn)(void), uintptr_t top)
{
	asm volatile("mv s1, sp\n"
		     "mv sp, %1\n"
		     "jalr %0\n"
		     "mv sp, s1\n"
		     :
		     : "r"(fn), "r"(top)
		     : "s1", "ra", "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7", "t0", "t1", "t2", "t3", "t4", "t5",
		       "t6", "memory");
}

static void display_task(void)
{
	sunxi_display_t *disp = (sunxi_display_t *)DISP_STATE;
	uint32_t w, h;

	if (sunxi_display_dt_read_alias(disp, "display0") != DRIVER_OK) {
		pr_err("display: invalid devicetree configuration\n");
		return;
	}
	if (sunxi_display_init(disp) != DRIVER_OK)
		return;

	w = disp->panel.timing.hactive;
	h = disp->panel.timing.vactive;
	draw_test_picture((uint32_t *)FB_BASE, w, h, w);
	flush_dcache_range(FB_BASE, FB_BASE + w * h * 4U);

	if (sunxi_display_set_fb(disp, FB_BASE, w, h, w * 4U, SUNXI_DE_FMT_XRGB8888) != DRIVER_OK)
		return;
	if (sunxi_display_enable(disp) != DRIVER_OK)
		return;
	sunxi_display_dump(disp);	/* register dump: printed only with CONFIG_DRIVER_DISPLAY_LOG_DEBUG */

	/* TCON colour bar (no DE involved) for 8 s, then the frame buffer through the DE */
	pr_info("display: TCON colour bar for 8 s\n");
	sunxi_display_set_pattern(disp, SUNXI_TCON_PATTERN_COLORBAR);
	mdelay(8000);
	sunxi_display_set_pattern(disp, SUNXI_TCON_PATTERN_NONE);
	pr_info("display: DE frame buffer, %ux%u running\n", (unsigned int)w, (unsigned int)h);
}

int main(void)
{
#ifndef CONFIG_APP_DRAM
	sunxi_psram_t psram = { 0 };
#endif

	if (sunxi_serial_init_stdout() != 0)
		return -1;
	show_banner();
	sunxi_clk_init();

#ifdef CONFIG_APP_DRAM
	/* running from the PSRAM: it is up already */
#else
	if (sunxi_psram_dt_read_alias(&psram, "psram0") != DRIVER_OK) {
		pr_err("PSRAM: invalid devicetree configuration\n");
		return -1;
	}
	sunxi_psram_init(&psram);
#endif

	call_on_stack(display_task, PSRAM_STACK_TOP);

	for (;;)
		mdelay(1000);
	return 0;
}
