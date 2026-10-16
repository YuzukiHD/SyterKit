/* SPDX-License-Identifier: GPL-2.0+ */
#include <io.h>
#include <stdint.h>

#include <drivers/display/display.h>

#define CCU_BASE	      0x02001000U
#define CCU_MBUS_SW_CFG_GATE  (CCU_BASE + 0x804U) /* bit 19: video out */
#define CCU_AHB_SW_CFG_GATE   (CCU_BASE + 0xe04U) /* bit 3: video out */

/*
 * The vendor driver ungates the video output register buses as optional "PPU" clocks. Both are already on in
 * FEL after reset, so this only matters when something has turned them off.
 */
static void sun252iw1_soc_init(void)
{
	setbits_le32(CCU_AHB_SW_CFG_GATE, 1U << 3);
	setbits_le32(CCU_MBUS_SW_CFG_GATE, 1U << 19);
}

/*
 * sun252iw1 (V861/V881): PLL_VIDEO and PLL_PERI are N = field + 1 multipliers of the 24 MHz oscillator
 * (1272..2520 MHz). PLL_VIDEO(4X) = PLL / (M + 1) with M in bit 0, which reads like the "reference / 2" bit
 * of the other chips. PLL_PERI(2X) = PLL / (M + 1) with M in bits 18:16, there is no reference divider.
 * The display engine does not take any SRAM, so the only init is ungating the video output register buses.
 */
const struct sunxi_disp_soc sunxi_disp_soc_sun252iw1 = {
	.compatible = "sun252iw1",
	.hosc_hz = 24000000U,
	.pll_video_n_shift = 8,
	.pll_video_n_width = 8,
	.pll_video_n_bias = 1,
	.pll_video_m_bit = 0,
	.pll_video_en_bit = 31,
	.pll_video_ldo_bit = 30,
	.pll_video_lock_en_bit = 29,
	.pll_video_locked_bit = 28,
	.pll_video_out_bit = 27,
	.pll_video_min_hz = 1272000000U,
	.pll_video_max_hz = 2520000000U,
	.pll_video_4x_div = 4,
	.pll_peri_n_shift = 8,
	.pll_peri_n_width = 8,
	.pll_peri_n_bias = 1,
	.pll_peri_p0_shift = 16,
	.pll_peri_p0_width = 3,
	.pll_peri_m_bit = 0xff,
	.soc_init = sun252iw1_soc_init,
};
