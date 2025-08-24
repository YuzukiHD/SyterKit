/* SPDX-License-Identifier: GPL-2.0+ */

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>

#include <sys-clk.h>

#include <sys-rproc.h>

static void sram_remap_set(int value) {
}

static void sunxi_hifi4_set_run_stall(uint32_t value) {
}

void sunxi_hifi4_clock_init(uint32_t addr) {
}

void sunxi_hifi4_start(void) {
}

void sunxi_hifi4_clock_reset(void) {
}

void sunxi_e906_clock_init(uint32_t addr) {
	uint32_t reg_val;

	/* rv cfg rst/gating */
	write32(RISCV_CFG_BGR_REG, RISCV_CFG_RST | RISCV_CFG_GATING);

	/* set start addr */
	write32(RISCV_STA_ADD_REG, addr);

	/* de-reset */
	reg_val = read32(RISCV_CFG_BGR_REG);
	reg_val |= RISCV_CORE_RST | RISCV_APB_DB_RST;
	write32(RISCV_CFG_BGR_REG, reg_val);

	/* turn on clock gating reset */
	reg_val = read32(RISCV_CLK_REG);
	reg_val |= RISCV_CLK_GATING;
	write32(RISCV_CLK_REG, reg_val);
}

void sunxi_e906_clock_reset(void) {
	uint32_t reg_val;

	/* De-assert PUBSRAM Clock and Gating */
	/* Make sure no program are using !!!! */
	reg_val = read32(RISCV_PUBSRAM_CFG_REG);
	reg_val |= RISCV_PUBSRAM_RST;
	reg_val |= RISCV_PUBSRAM_GATING;
	write32(RISCV_PUBSRAM_CFG_REG, reg_val);

	/* assert */
	write32(RISCV_CFG_BGR_REG, 0x0);
}

void dump_e906_clock(void) {
	uint32_t reg_val, pll_perf, factor_m, factor_n, pll_riscv, pll_dsp;
	uint32_t plln, pllm;
	uint8_t p0, p1;

	reg_val = read32(RISCV_CLK_REG);
	factor_m = (reg_val & 0x1F) + 1;
	factor_n = ((reg_val >> 8) & 0x3) + 1;
	pll_riscv = pll_perf / factor_m;

	printk_debug("CLK: RISC-V PLL FREQ=%uMHz\n", pll_riscv);
	printk_debug("CLK: RISC-V AXI FREQ=%uMHz\n", pll_riscv / factor_n);
}