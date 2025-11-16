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

uint32_t sunxi_clk_get_hosc_type() {
	return 24;
}

void sunxi_clk_reset(void) {
}

uint32_t sunxi_clk_get_peri1x_rate() {
	uint32_t reg32;
	uint8_t plln, pllm, p0;

	/* PLL PERI */
	reg32 = read32(SUNXI_CCU_BASE + PLL_PERI0_CTRL_REG);
	if (reg32 & (1 << 31)) {
		plln = ((reg32 >> 8) & 0xff) + 1;
		p0 = ((reg32 >> 20) & 0x03) + 1;
		pllm = ((reg32 >> 1) & 0x01) + 1;

		return ((((24 * plln) / (pllm * p0))) * 1000 * 1000);
	}

	return 0;
}
