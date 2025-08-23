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

void sunxi_e907_clock_init(uint32_t addr) {
	uint32_t reg_val;

	/* de-reset */
	reg_val = read32(CCU_RISCV_CFG_BGR_REG);
	reg_val |= CCU_RISCV_CFG_RST;
	reg_val |= CCU_RISCV_CFG_GATING;
	write32(CCU_RISCV_CFG_BGR_REG, reg_val);

	/* set start addr */
	reg_val = addr;
	write32(RISCV_STA_ADD_REG, reg_val);

	/* set e907 clock */
	reg_val = read32(CCU_RISCV_CLK_REG);
	reg_val &= ~(CCU_RISCV_CLK_MASK);
	reg_val |= CCU_RISCV_CLK_PERI_600M;
	write32(CCU_RISCV_CLK_REG, reg_val);

	/* turn on clock gating reset */
	reg_val = read32(CCU_RISCV_GATING_RST_REG);
	reg_val |= CCU_RISCV_CLK_GATING;
	reg_val |= CCU_RISCV_SOFT_RSTN;
	reg_val |= CCU_RISCV_SYS_APB_SOFT_RSTN;
	reg_val |= CCU_RISCV_GATING_RST_FIELD;
	write32(CCU_RISCV_GATING_RST_REG, reg_val);
}

void sunxi_e907_clock_reset(void) {
	uint32_t reg_val;

	/* turn off clk gating */
	reg_val = 0;
	reg_val |= CCU_RISCV_GATING_RST_FIELD;
	write32(CCU_RISCV_GATING_RST_REG, reg_val);

	/* assert */
	reg_val = read32(CCU_RISCV_CFG_BGR_REG);
	reg_val &= ~(CCU_RISCV_CFG_RST);
	reg_val &= ~(CCU_RISCV_CFG_GATING);
	write32(CCU_RISCV_CFG_BGR_REG, reg_val);
}

void dump_e907_clock(void) {
	uint32_t reg_val, pll_perf, factor_m, factor_n, pll_riscv;
	uint32_t plln, pllm;
	uint8_t p0, p1;

	/* PLL PERI */
	reg_val = read32(CCU_BASE + CCU_PLL_PERI_CTRL_REG);

	if (reg_val & (1 << 31)) {
		plln = ((reg_val >> 8) & 0xff) + 1;
		pllm = (reg_val & 0x01) + 1;
		p0 = ((reg_val >> 16) & 0x03) + 1;
		p1 = ((reg_val >> 20) & 0x03) + 1;

		pll_perf = (24 * plln) / (pllm * p0) >> 1;
	} else {
		printk_info("CLK: PLL_peri disabled\n");
		return;
	}

	reg_val = read32(CCU_RISCV_CLK_REG);
	factor_m = (reg_val & 0x1F) + 1;
	factor_n = ((reg_val >> 8) & 0x3) + 1;
	pll_riscv = pll_perf / factor_m;

	printk_info("CLK: RISC-V PLL FREQ=%uMHz\n", pll_riscv);
	printk_info("CLK: RISC-V AXI FREQ=%uMHz\n", pll_riscv / factor_n);
}