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
	uint32_t val = 0;

	val = readl(SUNXI_SYSCRL_BASE + SRAMC_SRAM_REMAP_REG);
	val &= ~(1 << BIT_SRAM_REMAP_ENABLE);
	val |= (value << BIT_SRAM_REMAP_ENABLE);
	writel(val, SUNXI_SYSCRL_BASE + SRAMC_SRAM_REMAP_REG);
}

static void sunxi_hifi4_set_run_stall(uint32_t value) {
	uint32_t reg_val = 0;

	reg_val = readl(DSP0_CFG_BASE + DSP_CTRL_REG0);
	reg_val &= ~(1 << BIT_RUN_STALL);
	reg_val |= (value << BIT_RUN_STALL);
	writel(reg_val, (DSP0_CFG_BASE + DSP_CTRL_REG0));
}

void sunxi_hifi4_clock_init(uint32_t addr) {
	uint32_t reg_val = 0;

	sram_remap_set(1);

	reg_val |= CCU_DSP_CLK_SRC_PERI2X;
	reg_val |= CCU_DSP_CLK_FACTOR_M(2);
	reg_val |= (1 << CCU_BIT_DSP_SCLK_GATING);
	writel(reg_val, CCU_BASE + CCU_DSP_CLK_REG);

	/* clock gating */
	reg_val = readl(CCU_BASE + CCU_DSP_BGR_REG);
	reg_val |= (1 << CCU_BIT_DSP0_CFG_GATING);
	writel(reg_val, CCU_BASE + CCU_DSP_BGR_REG);

	/* reset */
	reg_val = readl(CCU_BASE + CCU_DSP_BGR_REG);
	reg_val |= (1 << CCU_BIT_DSP0_CFG_RST);
	reg_val |= (1 << CCU_BIT_DSP0_DBG_RST);
	writel(reg_val, CCU_BASE + CCU_DSP_BGR_REG);

	/* set external Reset Vector if needed */
	if (addr != DSP_DEFAULT_RST_VEC) {
		writel(addr, DSP0_CFG_BASE + DSP_ALT_RESET_VEC_REG);

		reg_val = readl(DSP0_CFG_BASE + DSP_CTRL_REG0);
		reg_val |= (1 << BIT_START_VEC_SEL);
		writel(reg_val, DSP0_CFG_BASE + DSP_CTRL_REG0);
	}

	/* set runstall */
	sunxi_hifi4_set_run_stall(1);

	/* set dsp clken */
	reg_val = readl(DSP0_CFG_BASE + DSP_CTRL_REG0);
	reg_val |= (1 << BIT_DSP_CLKEN);
	writel(reg_val, DSP0_CFG_BASE + DSP_CTRL_REG0);

	/* de-assert dsp0 */
	reg_val = readl(CCU_BASE + CCU_DSP_BGR_REG);
	reg_val |= (1 << CCU_BIT_DSP0_RST);
	writel(reg_val, CCU_BASE + CCU_DSP_BGR_REG);

	/*
    reg_val = readl(CCU_BASE + CCU_DSP_CLK_REG);
    printk_info("CCU_DSP_CLK_REG = %x\n", reg_val);
    reg_val = readl(CCU_BASE + CCU_DSP_BGR_REG);
    printk_info("CCU_DSP_BGR_REG = %x\n", reg_val);
    reg_val = readl(DSP0_CFG_BASE + DSP_CTRL_REG0);
    printk_info("DSP_CTRL_REG0 = %x\n", reg_val);
*/
}

void sunxi_hifi4_start(void) {
	/* set dsp use local ram */
	sram_remap_set(0);

	/* clear runstall */
	sunxi_hifi4_set_run_stall(0);
}

void sunxi_hifi4_clock_reset(void) {
	uint32_t reg_val;

	/* assert */
	reg_val = readl(CCU_BASE + CCU_DSP_BGR_REG);
	reg_val &= ~(1 << CCU_BIT_DSP0_CFG_GATING);
	writel(reg_val, CCU_BASE + CCU_DSP_BGR_REG);

	reg_val = 0;
	writel(reg_val, CCU_BASE + CCU_DSP_BGR_REG);
}
