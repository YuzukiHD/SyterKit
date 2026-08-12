/* SPDX-License-Identifier: GPL-2.0+ */

#include <drivers/clk/clk.h>

void __attribute__((weak)) sunxi_clk_init(sunxi_ccu_t *ccu) {
	(void) ccu;
}

void __attribute__((weak)) sunxi_clk_pre_init(sunxi_ccu_t *ccu) {
	(void) ccu;
}

uint32_t __attribute__((weak)) sunxi_clk_get_hosc_type(sunxi_ccu_t *ccu) {
	(void) ccu;
	return 24;
}

void __attribute__((weak)) sunxi_clk_reset(sunxi_ccu_t *ccu) {
	(void) ccu;
}

void __attribute__((weak)) sunxi_clk_dump(sunxi_ccu_t *ccu) {
	(void) ccu;
}

void __attribute__((weak)) sunxi_usb_clk_deinit(sunxi_ccu_t *ccu) {
	(void) ccu;
}

void __attribute__((weak)) sunxi_usb_clk_init(sunxi_ccu_t *ccu) {
	(void) ccu;
}

uint32_t __attribute__((weak))
sunxi_clk_get_peri1x_rate(sunxi_ccu_t *ccu) {
	(void) ccu;
	return 0;
}

void __attribute__((weak))
sunxi_clk_set_cpu_pll(sunxi_ccu_t *ccu, uint32_t freq) {
	(void) ccu;
	(void) freq;
}
