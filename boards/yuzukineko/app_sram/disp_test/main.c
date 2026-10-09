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
#include <screenfetch.h>
#include <vt.h>

#include <dt-bindings/soc/sun252iw2.h>
#include <drivers/clk/clk.h>
#include <drivers/display/display.h>
#include <drivers/psram/psram.h>
#include <dt-compatible/display-dt.h>
#include <dt-compatible/psram-dt.h>

#define FB_BASE (SUNXI_PSRAM_BASE + 0x00800000U)
#define FB_SIZE 0x00780000U	/* up to the end of the 16 MB PSRAM, minus a margin */

/*
 * The display engine takes the SRAM above 0x2b000 away from the CPU (see platform/sun252iw2/soc.c): the image, its
 * bss and the stack have to stay below that, so the display state and the stack live in the PSRAM.
 */
#define PSRAM_STACK_TOP (SUNXI_PSRAM_BASE + 0x00400000U)
#define DISP_STATE	(SUNXI_PSRAM_BASE + 0x00300000U)
#define VT_EARLY_MEM	(SUNXI_PSRAM_BASE + 0x00200000U)
#define VT_EARLY_SIZE	0x8000U

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

/* The console buffer is taller than the panel: scrolling moves the shown window and rarely copies. */
#define VT_EXTRA_ROWS 1024U

static void vt_set_origin(void *ctx, uint32_t y)
{
	sunxi_display_t *disp = ctx;

	sunxi_display_set_fb(disp, FB_BASE + y * disp->fb_stride, disp->fb_width, disp->fb_height,
			     disp->fb_stride, SUNXI_DE_FMT_XRGB8888);
}

static void display_setup(void)
{
	sunxi_display_t *disp = (sunxi_display_t *)DISP_STATE;
	uint32_t w, h, total;
	vt_config_t vt = { 0 };

	if (sunxi_display_dt_read_alias(disp, "display0") != DRIVER_OK) {
		pr_err("display: invalid devicetree configuration\n");
		return;
	}
	if (sunxi_display_init(disp) != DRIVER_OK)
		return;

	w = disp->panel.timing.hactive;
	h = disp->panel.timing.vactive;
	total = h + VT_EXTRA_ROWS;
	if (total > FB_SIZE / (w * 4U))
		total = FB_SIZE / (w * 4U);

	vt.fb = FB_BASE;
	vt.width = w;
	vt.height = h;
	vt.stride = w * 4U;
	vt.total_height = total;
	vt.set_origin = vt_set_origin;
	vt.ctx = disp;
	if (vt_init(&vt) != 0) {
		pr_err("vt: init failed\n");
		return;
	}

	if (sunxi_display_set_fb(disp, FB_BASE + vt_origin() * w * 4U, w, h, w * 4U, SUNXI_DE_FMT_XRGB8888) != DRIVER_OK)
		return;
	if (sunxi_display_enable(disp) != DRIVER_OK)
		return;

	screenfetch(true);

}

/*
 * Never return to the SRAM stack: its top is the part of the SRAM the display engine owns, so any call made on it
 * after the display is up writes into the display engine registers.
 */
static void display_task(void)
{
	display_setup();
	for (;;)
		mdelay(1000);
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

	/* the text logged so far (and from now on) must not stay in the SRAM the display engine takes over */
	vt_set_memory((void *)VT_EARLY_MEM, VT_EARLY_SIZE);

	call_on_stack(display_task, PSRAM_STACK_TOP);

	return 0;
}
