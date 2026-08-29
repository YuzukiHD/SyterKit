/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file clk.c
 * @brief Weak default clock framework hooks for the sunxi SoC family.
 *
 * Each routine in this file is a deliberately empty weak implementation that
 * boards and SoC-specific code may override to perform clock pre-initialization,
 * main initialization, reset, or debug dumping.
 */

#include <drivers/clk/clk.h>

#include <stddef.h>

/**
 * @brief Perform any clock work required before the main clock init.
 */
void __attribute__((weak)) sunxi_clk_preinit(void)
{
}

/**
 * @brief Initialize the SoC clock tree.
 */
void __attribute__((weak)) sunxi_clk_init(void)
{
}

/**
 * @brief Reset the SoC clock tree to a known state.
 */
void __attribute__((weak)) sunxi_clk_reset(void)
{
}

/**
 * @brief Dump the current state of the SoC clock tree.
 */
void __attribute__((weak)) sunxi_clk_dump(void)
{
}
