/* SPDX-License-Identifier: Apache-2.0 */

#ifndef __SUN8IW20_SYS_RPROC_H__
#define __SUN8IW20_SYS_RPROC_H__

#include "reg/reg-rproc.h"

/**
 * Initialize the clock for the sunxi hifi4 module.
 *
 * @param addr The address of the hifi4 code.
 */
void sunxi_hifi4_clock_init(uint32_t addr);

/**
 * Start the sunxi hifi4 module.
 */
void sunxi_hifi4_start(void);

/**
 * Reset the clock for the sunxi hifi4.
 */
void sunxi_hifi4_clock_reset(void);

/**
 * Initialize the clock for the sunxi c906 module.
 *
 * @param addr The address of the c906 code.
 */
void sunxi_c906_clock_init(uint32_t addr);

/**
 * Reset the clock for the sunxi c906 module.
 */
void sunxi_c906_clock_reset(void);

#endif// __SUN8IW20_SYS_RPROC_H__