/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file clk.h
 * @brief Clock driver entry points for the Allwinner sun55iw3 SoC.
 */

#ifndef __SUN55IW3_CLK_H__
#define __SUN55IW3_CLK_H__

#include <stdint.h>

#include <drivers/clk/clk.h>

/**
 * @brief Set the sun55iw3 CPU PLL frequency.
 */
void sun55iw3_clk_set_cpu_pll(uint32_t freq);

#endif /* __SUN55IW3_CLK_H__ */
