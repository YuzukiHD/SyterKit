/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __SUN55IW3_CLK_H__
#define __SUN55IW3_CLK_H__

#include <stdint.h>

#include <drivers/clk/clk.h>

void sun55iw3_clk_set_cpu_pll(uint32_t freq);

#endif /* __SUN55IW3_CLK_H__ */
