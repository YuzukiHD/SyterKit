/* SPDX-License-Identifier: GPL-2.0+ */

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>

extern uint32_t current_hosc_freq;

void __attribute__((weak)) sunxi_clk_init(void) {
}

void __attribute__((weak)) sunxi_clk_pre_init(void) {
}

uint32_t __attribute__((weak)) sunxi_clk_get_hosc_type() {
	return 24;
}

void __attribute__((weak)) sunxi_clk_reset(void) {
}

void __attribute__((weak)) sunxi_clk_dump(void) {
}

void __attribute__((weak)) sunxi_usb_clk_deinit(void) {
}

void __attribute__((weak)) sunxi_usb_clk_init(void) {
}

uint32_t __attribute__((weak)) sunxi_clk_get_peri1x_rate() {
	return 0;
}

void __attribute__((weak)) sunxi_clk_set_cpu_pll(uint32_t freq) {
}
