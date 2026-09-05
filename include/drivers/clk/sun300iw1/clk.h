/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __SUN300IW1_CLK_H__
#define __SUN300IW1_CLK_H__

#include <stdint.h>

uint32_t sun300iw1_clk_get_hosc_rate(void);

/** @brief Apply Sun300IW1 clock setup required before the console starts. */
void sunxi_clk_preinit(void);

#endif /* __SUN300IW1_CLK_H__ */
