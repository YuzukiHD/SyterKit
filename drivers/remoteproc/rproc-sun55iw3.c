/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdint.h>

#include <driver.h>
#include <drivers/remoteproc/remoteproc.h>
#include <dt2c/driver.h>
#include <io.h>
#include <log.h>

#include <drivers/clk/sun55iw3/reg.h>

#define SUN55IW3_E906_PUBSRAM_CFG_OFFSET 0x0114U
#define SUN55IW3_E906_CLK_OFFSET 0x0120U
#define SUN55IW3_E906_CFG_BGR_OFFSET 0x0124U
#define SUN55IW3_E906_START_OFFSET 0x0204U

enum sun55iw3_e906_register {
	SUN55IW3_E906_DSP_PRCM,
	SUN55IW3_E906_CFG,
};

static int sun55iw3_e906_start(sunxi_remoteproc_t *remoteproc) {
	uint32_t value;
	uintptr_t dsp_prcm = remoteproc->registers[SUN55IW3_E906_DSP_PRCM].base;
	uintptr_t cfg = remoteproc->registers[SUN55IW3_E906_CFG].base;

	write32(dsp_prcm + SUN55IW3_E906_CFG_BGR_OFFSET,
		RISCV_CFG_RST | RISCV_CFG_GATING);
	write32(cfg + SUN55IW3_E906_START_OFFSET,
		(uint32_t) remoteproc->entry);
	value = read32(dsp_prcm + SUN55IW3_E906_CFG_BGR_OFFSET);
	value |= RISCV_CORE_RST | RISCV_APB_DB_RST;
	write32(dsp_prcm + SUN55IW3_E906_CFG_BGR_OFFSET, value);
	value = read32(dsp_prcm + SUN55IW3_E906_CLK_OFFSET);
	value |= RISCV_CLK_GATING;
	write32(dsp_prcm + SUN55IW3_E906_CLK_OFFSET, value);
	return DRIVER_OK;
}

static int sun55iw3_e906_reset(sunxi_remoteproc_t *remoteproc) {
	uint32_t value;
	uintptr_t dsp_prcm = remoteproc->registers[SUN55IW3_E906_DSP_PRCM].base;

	value = read32(dsp_prcm + SUN55IW3_E906_PUBSRAM_CFG_OFFSET);
	value |= RISCV_PUBSRAM_RST | RISCV_PUBSRAM_GATING;
	write32(dsp_prcm + SUN55IW3_E906_PUBSRAM_CFG_OFFSET, value);
	write32(dsp_prcm + SUN55IW3_E906_CFG_BGR_OFFSET, 0U);
	return DRIVER_OK;
}

static void sun55iw3_e906_dump(const sunxi_remoteproc_t *remoteproc) {
	uint32_t factor_m;
	uint32_t factor_n;
	uint32_t value;
	uintptr_t dsp_prcm = remoteproc->registers[SUN55IW3_E906_DSP_PRCM].base;

	value = read32(dsp_prcm + SUN55IW3_E906_CLK_OFFSET);
	factor_m = (value & 0x1fU) + 1U;
	factor_n = ((value >> 8) & 0x3U) + 1U;
	printk_debug("CLK: RISC-V reg=0x%08x, source=%u, core-div=%u, axi-div=%u\n",
		     value, (value >> 24) & 0x7U, factor_m, factor_n);
}

const sunxi_remoteproc_ops_t sunxi_remoteproc_ops = {
	.reset = sun55iw3_e906_reset,
	.start = sun55iw3_e906_start,
	.dump = sun55iw3_e906_dump,
};

DT2C_DRIVER_COMPAT("allwinner,sun55iw3-e906");
