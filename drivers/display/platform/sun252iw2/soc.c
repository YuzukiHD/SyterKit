/* SPDX-License-Identifier: GPL-2.0+ */
#include <io.h>
#include <stdint.h>

#include <drivers/display/display.h>

#define SYSCTRL_BASE 0x03000000U
#define SID_BASE     0x03006000U

/*
 * The DSI/LVDS pad termination is trimmed per chip: a 4 bit code is fused in
 * SID word 0x18 bits [7:4] and has to be copied, with an unlock key, into the
 * system control resistor register.
 */
static void sun252iw2_dphy_calibrate(void)
{
	uint32_t code = readl(SID_BASE + 0x18);

	if (!code)
		return;
	code = (code >> 4) & 0xf;
	writel(code | (code << 8) | (0x1937U << 16), SYSCTRL_BASE + 0x164);
}

/*
 * Gives the SRAM from 0x2b000 up to the display engine (its register file lives there). The CPU loses that part, so an
 * image running from the SRAM has to keep its code, data, bss and stack below 0x2b000.
 */
static void sun252iw2_soc_init(void)
{
	writel(0x0b000000, SYSCTRL_BASE + 0x04);
}

const struct sunxi_disp_soc sunxi_disp_soc_sun252iw2 = {
	.compatible = "sun252iw2",
	.hosc_hz = 24000000U,
	.pll_video_n_shift = 8,
	.pll_video_n_width = 8,
	.pll_video_n_bias = 1,
	.pll_video_m_bit = 1,
	.pll_video_en_bit = 31,
	.pll_video_ldo_bit = 30,
	.pll_video_lock_en_bit = 29,
	.pll_video_locked_bit = 28,
	.pll_video_out_bit = 27,
	.pll_video_min_hz = 288000000U,
	.pll_video_max_hz = 2400000000U,
	.pll_video_4x_div = 4,
	.pll_peri_n_shift = 8,
	.pll_peri_n_width = 8,
	.pll_peri_n_bias = 1,
	.pll_peri_p0_shift = 16,
	.pll_peri_p0_width = 3,
	.pll_peri_m_bit = 1,
	.soc_init = sun252iw2_soc_init,
	.dphy_calibrate = sun252iw2_dphy_calibrate,
};
