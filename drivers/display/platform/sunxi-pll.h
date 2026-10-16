/* SPDX-License-Identifier: GPL-2.0+ */
#ifndef __DISPLAY_PLATFORM_SUNXI_PLL_H__
#define __DISPLAY_PLATFORM_SUNXI_PLL_H__

#include <stdbool.h>
#include <stdint.h>

/*
 * Register layout of one N/M type PLL, for the chips that build the display clock sources from such PLLs:
 * rate = hosc / (m_bit set ? 2 : 1) / (post divider field + 1) * (N field + n_bias). Chip files own the
 * descriptors and decide which clock source ids map to which PLL; the common display code never sees this.
 */
struct sunxi_pll_nm {
	uint8_t n_shift, n_width, n_bias;
	uint8_t m_bit; /* set: the reference is halved, 0xff: none */
	uint8_t p_shift, p_width; /* post divider field (+1), width 0: none */
	uint8_t en_bit, ldo_bit, lock_en_bit, locked_bit, out_bit; /* 0xff: none */
	uint32_t min_hz, max_hz; /* range of set() */
};

#define SUNXI_PLL_NO_BIT 0xffU

uint32_t sunxi_pll_nm_rate(uintptr_t reg, uint32_t hosc_hz, const struct sunxi_pll_nm *pll);
/* hosc multiple closest to @hz inside the range, the reference divider is cleared so the rate is exact */
int sunxi_pll_nm_set(uintptr_t reg, uint32_t hosc_hz, const struct sunxi_pll_nm *pll, uint32_t hz);
void sunxi_pll_nm_enable(uintptr_t reg, const struct sunxi_pll_nm *pll, bool on);

#endif
