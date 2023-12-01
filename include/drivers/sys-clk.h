/* SPDX-License-Identifier: Apache-2.0 */

#ifndef __SUNXI_CLK_H__
#define __SUNXI_CLK_H__

#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <types.h>
#include <io.h>

#include "common.h"
#include "log.h"

#include "reg-ccu.h"

/* Init SoC Clock */
void sunxi_clk_init(void);

uint32_t sunxi_clk_get_peri1x_rate(void);

void sunxi_clk_dump(void);

#endif
