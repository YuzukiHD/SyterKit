/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdint.h>

#include <driver.h>
#include <drivers/remoteproc/remoteproc.h>
#include <dt2c/driver.h>
#include <io.h>
#include <log.h>

#include <drivers/clk/sun8iw20/reg.h>

#include "hifi4-reg.h"

enum sun8iw20_hifi4_register {
	SUN8IW20_HIFI4_SYSCTRL,
	SUN8IW20_HIFI4_DSP_CFG,
	SUN8IW20_HIFI4_CCU,
};

enum sun8iw20_c906_register {
	SUN8IW20_C906_CCU,
	SUN8IW20_C906_CFG,
};

static void sun8iw20_hifi4_sram_remap(sunxi_remoteproc_t *remoteproc,
					      bool local) {
	uint32_t value;
	uintptr_t sysctrl = remoteproc->registers[SUN8IW20_HIFI4_SYSCTRL].base;

	value = readl(sysctrl + SRAMC_SRAM_REMAP_REG);
	if (local)
		value &= ~(1U << BIT_SRAM_REMAP_ENABLE);
	else
		value |= 1U << BIT_SRAM_REMAP_ENABLE;
	writel(value, sysctrl + SRAMC_SRAM_REMAP_REG);
}

static void sun8iw20_hifi4_set_run_stall(sunxi_remoteproc_t *remoteproc,
						 bool stall) {
	uint32_t value;
	uintptr_t dsp_cfg = remoteproc->registers[SUN8IW20_HIFI4_DSP_CFG].base;

	value = readl(dsp_cfg + DSP_CTRL_REG0);
	value &= ~(1U << BIT_RUN_STALL);
	value |= (uint32_t) stall << BIT_RUN_STALL;
	writel(value, dsp_cfg + DSP_CTRL_REG0);
}

static int sun8iw20_hifi4_prepare(sunxi_remoteproc_t *remoteproc) {
	uint32_t value = 0U;
	uintptr_t ccu = remoteproc->registers[SUN8IW20_HIFI4_CCU].base;
	uintptr_t dsp_cfg = remoteproc->registers[SUN8IW20_HIFI4_DSP_CFG].base;

	sun8iw20_hifi4_sram_remap(remoteproc, false);
	value |= CCU_DSP_CLK_SRC_PERI2X;
	value |= CCU_DSP_CLK_FACTOR_M(2);
	value |= 1U << CCU_BIT_DSP_SCLK_GATING;
	writel(value, ccu + CCU_DSP_CLK_REG);

	value = readl(ccu + CCU_DSP_BGR_REG);
	value |= 1U << CCU_BIT_DSP0_CFG_GATING;
	writel(value, ccu + CCU_DSP_BGR_REG);

	value = readl(ccu + CCU_DSP_BGR_REG);
	value |= 1U << CCU_BIT_DSP0_CFG_RST;
	value |= 1U << CCU_BIT_DSP0_DBG_RST;
	writel(value, ccu + CCU_DSP_BGR_REG);

	if (remoteproc->entry != DSP_DEFAULT_RST_VEC) {
		writel((uint32_t) remoteproc->entry,
		       dsp_cfg + DSP_ALT_RESET_VEC_REG);
		value = readl(dsp_cfg + DSP_CTRL_REG0);
		value |= 1U << BIT_START_VEC_SEL;
		writel(value, dsp_cfg + DSP_CTRL_REG0);
	}

	sun8iw20_hifi4_set_run_stall(remoteproc, true);
	value = readl(dsp_cfg + DSP_CTRL_REG0);
	value |= 1U << BIT_DSP_CLKEN;
	writel(value, dsp_cfg + DSP_CTRL_REG0);

	value = readl(ccu + CCU_DSP_BGR_REG);
	value |= 1U << CCU_BIT_DSP0_RST;
	writel(value, ccu + CCU_DSP_BGR_REG);
	return DRIVER_OK;
}

static int sun8iw20_hifi4_start(sunxi_remoteproc_t *remoteproc) {
	sun8iw20_hifi4_sram_remap(remoteproc, true);
	sun8iw20_hifi4_set_run_stall(remoteproc, false);
	return DRIVER_OK;
}

static int sun8iw20_hifi4_reset(sunxi_remoteproc_t *remoteproc) {
	uint32_t value;
	uintptr_t ccu = remoteproc->registers[SUN8IW20_HIFI4_CCU].base;

	value = readl(ccu + CCU_DSP_BGR_REG);
	value &= ~(1U << CCU_BIT_DSP0_CFG_GATING);
	writel(value, ccu + CCU_DSP_BGR_REG);
	writel(0U, ccu + CCU_DSP_BGR_REG);
	return DRIVER_OK;
}

static int sun8iw20_c906_start(sunxi_remoteproc_t *remoteproc) {
	uint32_t value;
	uintptr_t ccu = remoteproc->registers[SUN8IW20_C906_CCU].base;
	uintptr_t cfg = remoteproc->registers[SUN8IW20_C906_CFG].base;

	value = CCU_RISCV_CFG_RST | CCU_RISCV_CFG_GATING;
	writel(value, ccu + CCU_RISCV_CFG_BGR_REG);
	writel((uint32_t) remoteproc->entry, cfg + 0x0004U);
	writel(0U, cfg + 0x0008U);

	value = readl(ccu + CCU_RISCV_CLK_REG);
	value &= ~CCU_RISCV_CLK_MASK;
	value |= CCU_RISCV_CLK_PERI_800M;
	writel(value, ccu + CCU_RISCV_CLK_REG);

	value = CCU_RISCV_RST_KEY_FIELD | CCU_RISCV_RST_SOFT_RSTN;
	writel(value, ccu + CCU_RISCV_RST_REG);
	return DRIVER_OK;
}

static int sun8iw20_c906_reset(sunxi_remoteproc_t *remoteproc) {
	uintptr_t ccu = remoteproc->registers[SUN8IW20_C906_CCU].base;

	writel(CCU_RISCV_CLK_GATING | CCU_RISCV_GATING_FIELD,
	       ccu + CCU_RISCV_GATING_RST_REG);
	writel(0U, ccu + CCU_RISCV_CFG_BGR_REG);
	return DRIVER_OK;
}

static void sun8iw20_c906_dump(const sunxi_remoteproc_t *remoteproc) {
	uint32_t factor_m;
	uint32_t factor_n;
	uint32_t pll_peripheral;
	uint32_t pll_riscv;
	uint32_t pllm;
	uint32_t plln;
	uint32_t value;
	uint8_t p1;
	uintptr_t ccu = remoteproc->registers[SUN8IW20_C906_CCU].base;

	value = read32(ccu + CCU_PLL_PERI0_CTRL_REG);
	if ((value & (1U << 31)) == 0U) {
		printk_info("CLK: PLL_peri disabled\n");
		return;
	}
	plln = ((value >> 8) & 0xffU) + 1U;
	pllm = (value & 0x01U) + 1U;
	p1 = (uint8_t) (((value >> 20) & 0x03U) + 1U);
	pll_peripheral = (24U * plln) / (pllm * p1);

	value = read32(ccu + CCU_RISCV_CLK_REG);
	factor_m = (value & 0x1fU) + 1U;
	factor_n = ((value >> 8) & 0x3U) + 1U;
	pll_riscv = pll_peripheral / factor_m;
	printk_info("CLK: RISC-V PLL FREQ=%uMHz\n", pll_riscv);
	printk_info("CLK: RISC-V AXI FREQ=%uMHz\n", pll_riscv / factor_n);
	printk_info("CLK: PERI1X = %uMHz\n", pll_peripheral);
}

#if defined(CONFIG_DRIVER_REMOTEPROC_SUN8IW20_C906)
const sunxi_remoteproc_ops_t sunxi_remoteproc_ops = {
	.reset = sun8iw20_c906_reset,
	.start = sun8iw20_c906_start,
	.dump = sun8iw20_c906_dump,
};
#else
const sunxi_remoteproc_ops_t sunxi_remoteproc_ops = {
	.reset = sun8iw20_hifi4_reset,
	.prepare = sun8iw20_hifi4_prepare,
	.start = sun8iw20_hifi4_start,
};
#endif

DT2C_DRIVER_COMPAT("allwinner,sun8iw20-hifi4");
DT2C_DRIVER_COMPAT("allwinner,sun8iw20-c906");
