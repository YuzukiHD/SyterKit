/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdint.h>

#include <driver.h>
#include <drivers/remoteproc/remoteproc.h>
#include <dt2c/driver.h>
#include <io.h>
#include <log.h>

#include <drivers/clk/sun8iw21/reg.h>

#define SUN8IW21_RISCV_CLK_OFFSET 0x0d00U
#define SUN8IW21_RISCV_GATING_RST_OFFSET 0x0d04U
#define SUN8IW21_RISCV_CFG_BGR_OFFSET 0x0d0cU
#define SUN8IW21_RISCV_START_OFFSET 0x0204U

enum sun8iw21_e907_register {
	SUN8IW21_E907_CCU,
	SUN8IW21_E907_CFG,
};

static int sun8iw21_e907_start(sunxi_remoteproc_t *remoteproc) {
	uint32_t value;
	uintptr_t ccu = remoteproc->registers[SUN8IW21_E907_CCU].base;
	uintptr_t cfg = remoteproc->registers[SUN8IW21_E907_CFG].base;

	value = read32(ccu + SUN8IW21_RISCV_CFG_BGR_OFFSET);
	value |= CCU_RISCV_CFG_RST | CCU_RISCV_CFG_GATING;
	write32(ccu + SUN8IW21_RISCV_CFG_BGR_OFFSET, value);
	write32(cfg + SUN8IW21_RISCV_START_OFFSET,
		(uint32_t) remoteproc->entry);

	value = read32(ccu + SUN8IW21_RISCV_CLK_OFFSET);
	value &= ~CCU_RISCV_CLK_MASK;
	value |= CCU_RISCV_CLK_PERI_600M;
	write32(ccu + SUN8IW21_RISCV_CLK_OFFSET, value);

	value = read32(ccu + SUN8IW21_RISCV_GATING_RST_OFFSET);
	value |= CCU_RISCV_CLK_GATING | CCU_RISCV_SOFT_RSTN |
		 CCU_RISCV_SYS_APB_SOFT_RSTN | CCU_RISCV_GATING_RST_FIELD;
	write32(ccu + SUN8IW21_RISCV_GATING_RST_OFFSET, value);
	return DRIVER_OK;
}

static int sun8iw21_e907_reset(sunxi_remoteproc_t *remoteproc) {
	uint32_t value;
	uintptr_t ccu = remoteproc->registers[SUN8IW21_E907_CCU].base;

	write32(ccu + SUN8IW21_RISCV_GATING_RST_OFFSET,
		CCU_RISCV_GATING_RST_FIELD);
	value = read32(ccu + SUN8IW21_RISCV_CFG_BGR_OFFSET);
	value &= ~(CCU_RISCV_CFG_RST | CCU_RISCV_CFG_GATING);
	write32(ccu + SUN8IW21_RISCV_CFG_BGR_OFFSET, value);
	return DRIVER_OK;
}

static void sun8iw21_e907_dump(const sunxi_remoteproc_t *remoteproc) {
	uint32_t factor_m;
	uint32_t factor_n;
	uint32_t pll_peripheral;
	uint32_t pll_riscv;
	uint32_t pllm;
	uint32_t plln;
	uint32_t value;
	uint8_t p0;
	uintptr_t ccu = remoteproc->registers[SUN8IW21_E907_CCU].base;

	value = read32(ccu + CCU_PLL_PERI_CTRL_REG);
	if ((value & (1U << 31)) == 0U) {
		printk_info("CLK: PLL_peri disabled\n");
		return;
	}
	plln = ((value >> 8) & 0xffU) + 1U;
	pllm = (value & 0x01U) + 1U;
	p0 = (uint8_t) (((value >> 16) & 0x03U) + 1U);
	pll_peripheral = ((24U * plln) / (pllm * p0)) >> 1;

	value = read32(ccu + SUN8IW21_RISCV_CLK_OFFSET);
	factor_m = (value & 0x1fU) + 1U;
	factor_n = ((value >> 8) & 0x3U) + 1U;
	pll_riscv = pll_peripheral / factor_m;
	printk_info("CLK: RISC-V PLL FREQ=%uMHz\n", pll_riscv);
	printk_info("CLK: RISC-V AXI FREQ=%uMHz\n", pll_riscv / factor_n);
}

static const sunxi_remoteproc_ops_t sun8iw21_e907_ops = {
	.reset = sun8iw21_e907_reset,
	.start = sun8iw21_e907_start,
	.dump = sun8iw21_e907_dump,
};

int sunxi_remoteproc_bind(sunxi_remoteproc_t *remoteproc,
			  sunxi_remoteproc_variant_t variant) {
	if (remoteproc == NULL ||
	    variant != SUNXI_REMOTEPROC_VARIANT_SUN8IW21_E907 ||
	    remoteproc->register_count != 2U ||
	    remoteproc->registers[SUN8IW21_E907_CCU].size < 0xd10U ||
	    remoteproc->registers[SUN8IW21_E907_CFG].size < 0x208U)
		return DRIVER_ERROR_INVALID;
	remoteproc->ops = &sun8iw21_e907_ops;
	return DRIVER_OK;
}

DT2C_DRIVER_COMPAT("allwinner,sun8iw21-e907");
