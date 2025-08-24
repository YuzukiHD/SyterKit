/* SPDX-License-Identifier: GPL-2.0+ */

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <timer.h>

#include <log.h>

#include <sys-clk.h>


void sunxi_clk_init(void) {
}

void sunxi_clk_dump() {
}

/* we got hosc freq in arch/timer.c */
extern uint32_t current_hosc_freq;

uint32_t sunxi_clk_get_peri1x_rate() {
	return 192; /* PERI_192M */
}