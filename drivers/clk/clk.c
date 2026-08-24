/* SPDX-License-Identifier: GPL-2.0+ */

#include <drivers/clk/clk.h>

#include <stddef.h>

void __attribute__((weak)) sunxi_clk_preinit(void)
{
}

void __attribute__((weak)) sunxi_clk_init(void)
{
}

void __attribute__((weak)) sunxi_clk_reset(void)
{
}

void __attribute__((weak)) sunxi_clk_dump(void)
{
}
