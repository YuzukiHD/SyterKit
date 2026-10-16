/* SPDX-License-Identifier: GPL-2.0+ */

/*
 * Self test of the V861 display engine and TCON LCD (sun252iw1), run once for each of the three outputs preset
 * in the device tree: parallel RGB (display0), serial RGB (display1) and i8080 (display2).
 *
 * The board carries no panel, so every check is made on the SoC side: clocks, register contents, the TCON line
 * counter, the DE frame flag and the i8080 write handshake. The pads are never touched on purpose: the pin
 * list is cleared, and the panel power sequences and the backlight (plain GPIOs of an unverified wiring) are
 * dropped. What actually shows up on a panel is not verified here.
 */

#include <drivers/serial/serial.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <types.h>

#include <cache.h>
#include <common.h>
#include <io.h>
#include <log.h>
#include <timer.h>

#include <dt-bindings/soc/sun252iw1.h>
#include <drivers/clk/clk.h>
#include <drivers/display/display.h>
#include <drivers/dram/dram.h>
#include <drivers/i2c/i2c.h>
#include <drivers/pmu/axp.h>
#include <dt-compatible/display-dt.h>
#include <dt-compatible/dram-dt.h>
#include <dt-compatible/i2c-dt.h>

#define FB_BASE 0x45000000U

#define CCU_DE_CLK     (SUNXI_CCU_BASE + 0x600U)
#define CCU_DE_BUS     (SUNXI_CCU_BASE + 0x60cU)
#define CCU_TCON_TOP   (SUNXI_CCU_BASE + 0xabcU)
#define CCU_TCON_CLK   (SUNXI_CCU_BASE + 0xb60U)
#define CCU_TCON_BUS   (SUNXI_CCU_BASE + 0xb7cU)

#define DE_TOP	       SUNXI_DE_BASE
#define DE_GLB	       (SUNXI_DE_BASE + 0x100000U)
#define DE_BLD	       (SUNXI_DE_BASE + 0x101000U)
#define DE_UI	       (SUNXI_DE_BASE + 0x103000U)

#define TCON	       SUNXI_TCON_LCD0_BASE
#define TCON_GCTL      (TCON + 0x000U)
#define TCON_CTL       (TCON + 0x040U)
#define TCON_DCLK      (TCON + 0x044U)
#define TCON_BASIC0    (TCON + 0x048U)
#define TCON_BASIC1    (TCON + 0x04cU)
#define TCON_BASIC2    (TCON + 0x050U)
#define TCON_BASIC3    (TCON + 0x054U)
#define TCON_HV_CTL    (TCON + 0x058U)
#define TCON_CPU_CTL   (TCON + 0x060U)
#define TCON_TRI0      (TCON + 0x160U)
#define TCON_TRI1      (TCON + 0x164U)
#define TCON_TRI3      (TCON + 0x16cU)

sunxi_dram_t dram = { 0 };

static int failures;

#define CHECK(tag, name, cond, fmt, ...)                                                                      \
	do {                                                                                                  \
		bool ok_ = (cond);                                                                            \
		pr_info("TEST %-5s %-12s %s    " fmt "\n", tag, name, ok_ ? "PASS" : "FAIL", ##__VA_ARGS__);    \
		failures += !ok_;                                                                             \
	} while (0)

static void sunxi_pmc_config(void)
{
	if (!(readl(SUNXI_RTC_PMC_BYPASS_STATUS) & BIT(0)))
		writel(BIT(0) | BIT(1) | BIT(2) | BIT(5), SUNXI_RTC_IOMODE_CTL);
}

static uint32_t abs_diff(uint32_t a, uint32_t b)
{
	return a > b ? a - b : b - a;
}

/* register compare that reports the mismatches, returns how many there were */
static unsigned int reg_expect(const char *what, uintptr_t addr, uint32_t mask, uint32_t expect)
{
	uint32_t got = readl(addr) & mask;

	if (got == (expect & mask))
		return 0;
	pr_err("  %s @%08x: %08x, expected %08x (mask %08x)\n", what, (unsigned int)addr, (unsigned int)got,
		(unsigned int)(expect & mask), (unsigned int)mask);
	return 1;
}

static void fill_fb(const sunxi_display_t *disp)
{
	uint32_t *fb = (uint32_t *)FB_BASE;
	uint32_t w = disp->panel.timing.hactive, h = disp->panel.timing.vactive, x, y;

	for (y = 0; y < h; y++)
		for (x = 0; x < w; x++)
			fb[y * w + x] = 0xff000000U | ((x * 255U / w) << 16) | ((y * 255U / h) << 8) | ((x ^ y) & 0xffU);
	flush_dcache_all();
}

/* ------------------------------------------------------------------ */

static void test_clocks(const char *tag, sunxi_display_t *disp)
{
	uint32_t reg, rate, want = 300000000U, mod;

	/* display engine: gate, mux 0 = PLL_PERI(300M), 5 bit divider */
	reg = readl(CCU_DE_CLK);
	rate = sunxi_disp_src_rate(&disp->clk, SUNXI_DISP_SRC_PLL_PERI_300M) / ((reg & 0x1fU) + 1U);
	CHECK(tag, "de_clock", (reg & BIT(31)) && !(reg & BIT(24)) && abs_diff(rate, want) <= want / 20U,
		"reg %08x, %u Hz (want %u)", (unsigned int)reg, (unsigned int)rate, (unsigned int)want);

	/* TCON: gate, mux 1 = PLL_PERI(2X), M 3:0, P 9:8 */
	reg = readl(CCU_TCON_CLK);
	mod = sunxi_disp_src_rate(&disp->clk, SUNXI_DISP_SRC_PLL_PERI_2X) / ((reg & 0xfU) + 1U) >> ((reg >> 8) & 3U);
	CHECK(tag, "tcon_clock", (reg & BIT(31)) && ((reg >> 24) & 7U) == 1U && mod == disp->tcon.mod_hz,
		"reg %08x, module %u Hz (driver %u), pixel %u Hz (want %u)", (unsigned int)reg, (unsigned int)mod,
		(unsigned int)disp->tcon.mod_hz, (unsigned int)disp->tcon.pixclk_hz,
		(unsigned int)disp->panel.timing.pixel_clock_hz);

	CHECK(tag, "pixel_clock", abs_diff(disp->tcon.pixclk_hz, disp->panel.timing.pixel_clock_hz) <= disp->panel.timing.pixel_clock_hz / 20U,
		"%u Hz, requested %u Hz", (unsigned int)disp->tcon.pixclk_hz,
		(unsigned int)disp->panel.timing.pixel_clock_hz);

	CHECK(tag, "bus_gates",
		(readl(CCU_DE_BUS) & (BIT(0) | BIT(16))) == (BIT(0) | BIT(16)) &&
			(readl(CCU_TCON_TOP) & (BIT(0) | BIT(16))) == (BIT(0) | BIT(16)) &&
			(readl(CCU_TCON_BUS) & (BIT(0) | BIT(16))) == (BIT(0) | BIT(16)),
		"de %08x top %08x tcon %08x", (unsigned int)readl(CCU_DE_BUS), (unsigned int)readl(CCU_TCON_TOP),
		(unsigned int)readl(CCU_TCON_BUS));
}

static void test_de_regs(const char *tag, const sunxi_display_t *disp)
{
	uint32_t w = disp->panel.timing.hactive, h = disp->panel.timing.vactive;
	uint32_t size = ((h - 1U) << 16) | (w - 1U);
	unsigned int bad = 0;

	bad += reg_expect("top sclk", DE_TOP + 0x00, 1, 1);
	bad += reg_expect("top hclk", DE_TOP + 0x04, 1, 1);
	bad += reg_expect("top reset", DE_TOP + 0x08, 1, 1);
	bad += reg_expect("glb ctl", DE_GLB + 0x00, 1, 1);
	bad += reg_expect("glb size", DE_GLB + 0x0c, 0x1fff1fffU, size);
	bad += reg_expect("ui attr en", DE_UI + 0x00, 1, 1);
	bad += reg_expect("ui attr fmt", DE_UI + 0x00, 0x1f00U, 0x0400U); /* XRGB8888 */
	bad += reg_expect("ui size", DE_UI + 0x04, 0x1fff1fffU, size);
	bad += reg_expect("ui pitch", DE_UI + 0x0c, 0xffffffffU, w * 4U);
	bad += reg_expect("ui addr", DE_UI + 0x10, 0xffffffffU, FB_BASE);
	bad += reg_expect("ui ovl size", DE_UI + 0x88, 0x1fff1fffU, size);
	bad += reg_expect("bld pipe0 size", DE_BLD + 0x08, 0x1fff1fffU, size);
	bad += reg_expect("bld route", DE_BLD + 0x80, 0xfU, 1U); /* pipe 0 <- UI channel */
	bad += reg_expect("bld out size", DE_BLD + 0x8c, 0x1fff1fffU, size);
	CHECK(tag, "de_regs", bad == 0, "%u mismatch(es), %ux%u at %08x", bad, (unsigned int)w, (unsigned int)h, FB_BASE);
}

static void test_tcon_regs(const char *tag, const sunxi_display_t *disp)
{
	const sunxi_disp_timing_t *t = &disp->panel.timing;
	uint32_t hbp = t->hsync_len + t->hback_porch, vbp = t->vsync_len + t->vback_porch;
	bool cpu = disp->iface == SUNXI_DISP_IF_I8080;
	unsigned int bad = 0;

	bad += reg_expect("gctl en", TCON_GCTL, BIT(31), BIT(31));
	bad += reg_expect("ctl en", TCON_CTL, BIT(31), BIT(31));
	bad += reg_expect("ctl if", TCON_CTL, 3U << 24, (cpu ? 1U : 0U) << 24);
	bad += reg_expect("dclk en", TCON_DCLK, 0xfU << 28, 0xfU << 28);
	bad += reg_expect("dclk div", TCON_DCLK, 0x7fU, disp->tcon.div);
	bad += reg_expect("basic0", TCON_BASIC0, 0x0fff0fffU, ((t->hactive - 1U) << 16) | (t->vactive - 1U));
	bad += reg_expect("basic1", TCON_BASIC1, 0x1fff0fffU, ((t->htotal - 1U) << 16) | (hbp - 1U));
	bad += reg_expect("basic2", TCON_BASIC2, 0x1fff0fffU, ((t->vtotal * 2U) << 16) | (vbp - 1U));
	bad += reg_expect("basic3", TCON_BASIC3, 0x03ff03ffU, ((t->hsync_len - 1U) << 16) | (t->vsync_len - 1U));
	if (cpu) {
		bad += reg_expect("cpu mode", TCON_CPU_CTL, 0xfU << 28, (uint32_t)disp->if_cfg.cpu_mode << 28);
		if (disp->if_cfg.cpu_trigger) {
			bad += reg_expect("tri0 size", TCON_TRI0, 0xfffU, t->hactive - 1U);
			bad += reg_expect("tri1 num", TCON_TRI1, 0xffffU, t->vactive - 1U);
			bad += reg_expect("tri3 mode", TCON_TRI3, 3U << 28,
				(uint32_t)(disp->if_cfg.cpu_te == 0U ? 1U : disp->if_cfg.cpu_te + 1U) << 28);
		}
	} else {
		bad += reg_expect("hv mode", TCON_HV_CTL, 0xfU << 28, (uint32_t)disp->if_cfg.hv_mode << 28);
		bad += reg_expect("hv srgb seq", TCON_HV_CTL, 0xfU << 24, (uint32_t)disp->if_cfg.srgb_seq << 24);
	}
	CHECK(tag, "tcon_regs", bad == 0, "%u mismatch(es), htotal %u vtotal %u", bad, (unsigned int)t->htotal,
		(unsigned int)t->vtotal);
}

/* the line counter wraps once per frame: count the wraps over 500 ms and compare with the timing */
static void test_frame_rate(const char *tag, sunxi_display_t *disp)
{
	const sunxi_disp_timing_t *t = &disp->panel.timing;
	uint32_t start = time_us(), last = sunxi_tcon_get_line(&disp->tcon), line, frames = 0, elapsed;
	uint32_t mhz_expect = (uint32_t)(((uint64_t)disp->tcon.pixclk_hz * 1000U) / ((uint32_t)t->htotal * t->vtotal));
	uint32_t mhz_got;
	bool moved = false;

	while ((elapsed = time_us() - start) < 500000U) {
		line = sunxi_tcon_get_line(&disp->tcon);
		if (line != last)
			moved = true;
		if (line < last)
			frames++;
		last = line;
	}
	mhz_got = (uint32_t)(((uint64_t)frames * 1000000U * 1000U) / elapsed);
	CHECK(tag, "frame_rate", moved && abs_diff(mhz_got, mhz_expect) <= mhz_expect / 10U + 2000U,
		"%u frames in %u us = %u.%03u Hz, timing says %u.%03u Hz", (unsigned int)frames, (unsigned int)elapsed,
		(unsigned int)(mhz_got / 1000U), (unsigned int)(mhz_got % 1000U), (unsigned int)(mhz_expect / 1000U),
		(unsigned int)(mhz_expect % 1000U));
}

static void test_de_frames(const char *tag, sunxi_display_t *disp)
{
	bool flag;

	writel(1U, DE_GLB + 0x04); /* the frame end flag is cleared by writing 1 */
	mdelay(100);
	flag = readl(DE_GLB + 0x04) & 1U;
	CHECK(tag, "de_frames", flag, "glb status %08x after 100 ms", (unsigned int)readl(DE_GLB + 0x04));
	CHECK(tag, "underflow", !sunxi_tcon_check_underflow(&disp->tcon), "no TCON FIFO underflow");
}

/*
 * i8080: the bus handshake of a command write, then one manual frame trigger. Both only read status bits that are
 * 0 when the TCON does not answer at all, so they are only meaningful when the CPU control register reads back
 * the mode that was written.
 */
static void test_i8080(const char *tag, sunxi_display_t *disp)
{
	int ret;
	uint32_t start;
	bool busy, alive = ((readl(TCON_CPU_CTL) >> 28) & 0xfU) == disp->if_cfg.cpu_mode;

	ret = sunxi_tcon_cpu_write(&disp->tcon, false, 0x00); /* NOP */
	CHECK(tag, "cpu_write", alive && ret == DRIVER_OK && !(readl(TCON_CPU_CTL) & (BIT(23) | BIT(22))),
		"%swrite returned %d, cpu ctl %08x", alive ? "" : "TCON does not answer, ", ret,
		(unsigned int)readl(TCON_CPU_CTL));

	sunxi_tcon_trigger(&disp->tcon);
	start = time_us();
	while ((busy = readl(TCON_CPU_CTL) & BIT(1)) && time_us() - start < 100000U)
		;
	CHECK(tag, "trigger", alive && !busy, "%sframe trigger %s after %u us, cpu ctl %08x",
		alive ? "" : "TCON does not answer, ", busy ? "still busy" : "finished",
		(unsigned int)(time_us() - start), (unsigned int)readl(TCON_CPU_CTL));
}

static void run_output(const char *alias, const char *tag)
{
	sunxi_display_t disp;
	int ret;

	pr_info("---- %s (%s)\n", alias, tag);
	ret = sunxi_display_dt_read_alias(&disp, alias);
	CHECK(tag, "dt", ret == DRIVER_OK, "device tree read %d", ret);
	if (ret != DRIVER_OK)
		return;

	/* no pads, no panel GPIOs, no backlight: nothing is wired on this board */
	disp.pin_count = 0;
	memset(&disp.backlight, 0, sizeof(disp.backlight));
	disp.panel.power_on = (sunxi_disp_seq_t){ 0 };
	disp.panel.power_off = (sunxi_disp_seq_t){ 0 };
	disp.panel.enable_delay_ms = 0;

	ret = sunxi_display_init(&disp);
	CHECK(tag, "init", ret == DRIVER_OK, "init %d", ret);
	if (ret != DRIVER_OK)
		return;
	fill_fb(&disp);
	ret = sunxi_display_set_fb(&disp, FB_BASE, disp.panel.timing.hactive, disp.panel.timing.vactive,
		disp.panel.timing.hactive * 4U, SUNXI_DE_FMT_XRGB8888);
	CHECK(tag, "set_fb", ret == DRIVER_OK, "set_fb %d", ret);
	ret = sunxi_display_enable(&disp);
	CHECK(tag, "enable", ret == DRIVER_OK, "enable %d", ret);
	if (ret != DRIVER_OK)
		return;

	test_clocks(tag, &disp);
	test_de_regs(tag, &disp);
	test_tcon_regs(tag, &disp);
	if (disp.iface == SUNXI_DISP_IF_I8080) {
		test_i8080(tag, &disp);
	} else {
		test_frame_rate(tag, &disp);
		test_de_frames(tag, &disp);
	}
	sunxi_display_disable(&disp);
}

int main(void)
{
	axp_pmu_t pmu;
	sunxi_i2c_t i2c;

	if (sunxi_serial_init_stdout() != 0)
		return -1;
	show_banner();

	if (sunxi_i2c_dt_read_alias(&i2c, "i2c2") != DRIVER_OK || pmu_axp333_config(&pmu, &i2c) != DRIVER_OK) {
		pr_err("PMU: invalid devicetree configuration\n");
		return -1;
	}
	sunxi_clk_init();
	sunxi_pmc_config();
	sunxi_i2c_init(&i2c);
	pmu_axp333_init(&pmu);
	pmu_axp333_set_vol(&pmu, "dcdc2", 1500, 1);

	if (sunxi_dram_dt_read_alias(&dram, "dram0") != DRIVER_OK || sunxi_dram_init(&dram) == 0U) {
		pr_err("DRAM: initialization failed\n");
		return -1;
	}

	run_output("display0", "rgb");
	run_output("display1", "srgb");
	run_output("display2", "i8080");

	if (failures)
		pr_err("DISP: %d test(s) FAILED\n", failures);
	else
		pr_info("DISP: all tests passed (0 failures)\n");
	for (;;)
		__asm__ __volatile__("wfi");
}
