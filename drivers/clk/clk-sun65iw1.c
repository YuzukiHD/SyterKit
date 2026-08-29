/* SPDX-License-Identifier: GPL-2.0+ */
#define pr_fmt(fmt) "clk-sun65iw1: " fmt

/**
 * @file clk-sun65iw1.c
 * @brief Clock driver for the Allwinner sun65iw1 SoC.
 *
 * The sun65iw1 platform has no early clock setup requirements, so the public
 * clock hooks are provided as no-op stubs.
 */

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <timer.h>

#include <log.h>

#include <drivers/clk/clk.h>
#include <drivers/clk/sun65iw1/reg.h>

/**
 * @brief Initialize the SoC clocks.
 *
 * No clock configuration is required on sun65iw1, so this is an empty stub.
 */
void sunxi_clk_init(void)
{
}

/**
 * @brief Reset the SoC clocks to their default state.
 *
 * No clock reset is required on sun65iw1, so this is an empty stub.
 */
void sunxi_clk_reset(void)
{
}
