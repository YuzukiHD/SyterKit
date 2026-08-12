/* SPDX-License-Identifier: GPL-2.0+ */

#include <drivers/clk/clk.h>

void __attribute__((weak)) sunxi_clk_init(sunxi_ccu_t *ccu) {
	(void) ccu;
}

void __attribute__((weak)) sunxi_clk_reset(sunxi_ccu_t *ccu) {
	(void) ccu;
}

void __attribute__((weak)) sunxi_clk_dump(sunxi_ccu_t *ccu) {
	(void) ccu;
}
