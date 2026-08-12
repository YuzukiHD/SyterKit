/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>
#include <io.h>

#include <log.h>

#include <common.h>

#include <dt-bindings/soc/sun20iw1.h>
#include <drivers/clk/clk.h>

#include <mmu.h>

#include <drivers/spi/spi.h>
#include <drivers/serial/serial.h>

void clean_syterkit_data(void) {
}

void sys_reset(void) {
	write32(SUNXI_WDOG_BASE + 0x08, 0x16aa0001U);

	for (;;) {
	}
}
