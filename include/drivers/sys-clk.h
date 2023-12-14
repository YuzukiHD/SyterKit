/* SPDX-License-Identifier: Apache-2.0 */

#ifndef __SYS_CLK_H__
#define __SYS_CLK_H__

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include "common.h"
#include "log.h"

#include "reg-ccu.h"

/* Init SoC Clock */
void sunxi_clk_init(void);

void sunxi_clk_reset(void);

uint32_t sunxi_clk_get_peri1x_rate(void);

void sunxi_clk_dump(void);

#endif
