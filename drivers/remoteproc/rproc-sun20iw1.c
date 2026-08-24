/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdint.h>

#include <driver.h>
#include <drivers/remoteproc/remoteproc.h>
#include <dt2c/driver.h>
#include <io.h>

#include <drivers/clk/sun20iw1/reg.h>

#include "hifi4-reg.h"

enum sun20iw1_hifi4_register {
	SUN20IW1_HIFI4_SYSCTRL,
	SUN20IW1_HIFI4_DSP_CFG,
	SUN20IW1_HIFI4_CCU,
};

static void sun20iw1_hifi4_sram_remap(sunxi_remoteproc_t *remoteproc, bool local)
{
	uint32_t value;
	uintptr_t sysctrl = remoteproc->registers[SUN20IW1_HIFI4_SYSCTRL].base;

	value = readl(sysctrl + SRAMC_SRAM_REMAP_REG);
	value &= ~(1U << BIT_SRAM_REMAP_ENABLE);
	value |= (uint32_t)local << BIT_SRAM_REMAP_ENABLE;
	writel(value, sysctrl + SRAMC_SRAM_REMAP_REG);
}

static void sun20iw1_hifi4_set_run_stall(sunxi_remoteproc_t *remoteproc, bool stall)
{
	uint32_t value;
	uintptr_t dsp_cfg = remoteproc->registers[SUN20IW1_HIFI4_DSP_CFG].base;

	value = readl(dsp_cfg + DSP_CTRL_REG0);
	value &= ~(1U << BIT_RUN_STALL);
	value |= (uint32_t)stall << BIT_RUN_STALL;
	writel(value, dsp_cfg + DSP_CTRL_REG0);
}

static int sun20iw1_hifi4_prepare(sunxi_remoteproc_t *remoteproc)
{
	uint32_t value = 0U;
	uintptr_t ccu = remoteproc->registers[SUN20IW1_HIFI4_CCU].base;
	uintptr_t dsp_cfg = remoteproc->registers[SUN20IW1_HIFI4_DSP_CFG].base;

	sun20iw1_hifi4_sram_remap(remoteproc, true);
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
		writel((uint32_t)remoteproc->entry, dsp_cfg + DSP_ALT_RESET_VEC_REG);
		value = readl(dsp_cfg + DSP_CTRL_REG0);
		value |= 1U << BIT_START_VEC_SEL;
		writel(value, dsp_cfg + DSP_CTRL_REG0);
	}

	sun20iw1_hifi4_set_run_stall(remoteproc, true);
	value = readl(dsp_cfg + DSP_CTRL_REG0);
	value |= 1U << BIT_DSP_CLKEN;
	writel(value, dsp_cfg + DSP_CTRL_REG0);

	value = readl(ccu + CCU_DSP_BGR_REG);
	value |= 1U << CCU_BIT_DSP0_RST;
	writel(value, ccu + CCU_DSP_BGR_REG);
	return DRIVER_OK;
}

static int sun20iw1_hifi4_start(sunxi_remoteproc_t *remoteproc)
{
	sun20iw1_hifi4_sram_remap(remoteproc, false);
	sun20iw1_hifi4_set_run_stall(remoteproc, false);
	return DRIVER_OK;
}

static int sun20iw1_hifi4_reset(sunxi_remoteproc_t *remoteproc)
{
	uint32_t value;
	uintptr_t ccu = remoteproc->registers[SUN20IW1_HIFI4_CCU].base;

	value = readl(ccu + CCU_DSP_BGR_REG);
	value &= ~(1U << CCU_BIT_DSP0_CFG_GATING);
	writel(value, ccu + CCU_DSP_BGR_REG);
	writel(0U, ccu + CCU_DSP_BGR_REG);
	return DRIVER_OK;
}

const sunxi_remoteproc_ops_t sunxi_remoteproc_ops = {
	.reset = sun20iw1_hifi4_reset,
	.prepare = sun20iw1_hifi4_prepare,
	.start = sun20iw1_hifi4_start,
};

DT2C_DRIVER_COMPAT("allwinner,sun20iw1-hifi4");
