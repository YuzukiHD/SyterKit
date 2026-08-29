/* SPDX-License-Identifier: GPL-2.0+ */

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <timer.h>

#include <log.h>

#include <drivers/clk/clk.h>
#include <drivers/clk/sun252iw2/reg.h>

#define SUNXI_C907_CLK (600)

#define PLL_CPU_UPDATE_OFFSET (26)
#define PLL_CPU_LOCK_OFFSET (28)
#define PLL_CPU_LOCK_ENABLE_OFFSET (29)

/**
 * @brief Switch the CPU PLL to a given frequency.
 *
 * Frequencies are encoded as 24M * n / (m0 * m1 * p).  The F101 board
 * configuration selects 600MHz (n = 0x19) with m0 = m1 = 1 and p = 1.
 *
 * @param[in] reg_addr PLL_CPU control register address.
 * @param[in] n_factor N divider value.
 */
static void static_pll_switch_freq(uintptr_t reg_addr, uint32_t n_factor)
{
	uint32_t reg_val;

	/* set n=m0=m1=1,p=1: PLL_OUT = 24M * n / (m0 * m1 * p) */
	reg_val = readl(reg_addr);
	reg_val &= ~((0xffU << PLL_CPU_CTRL_REG_PLL_N_OFFSET) |
		     (0x7U << PLL_CPU_CTRL_REG_PLL_P_OFFSET) |
		     (0x3U << PLL_CPU_CTRL_REG_PLL_M0_OFFSET) |
		     (0xfU << PLL_CPU_CTRL_REG_PLL_M1_OFFSET));
	reg_val |= (n_factor << PLL_CPU_CTRL_REG_PLL_N_OFFSET) | (0x0U << PLL_CPU_CTRL_REG_PLL_M1_OFFSET);
	writel(reg_val, reg_addr);
	udelay(10);

	/* lock enable */
	reg_val = readl(reg_addr);
	reg_val &= ~(0x1U << PLL_CPU_LOCK_ENABLE_OFFSET);
	writel(reg_val, reg_addr);
	udelay(10);
	reg_val |= (0x1U << PLL_CPU_LOCK_ENABLE_OFFSET);
	writel(reg_val, reg_addr);

	/* update bit */
	reg_val = readl(reg_addr);
	reg_val |= (0x1U << PLL_CPU_UPDATE_OFFSET);
	writel(reg_val, reg_addr);
	do {
		reg_val = readl(reg_addr);
		reg_val &= (0x1U << PLL_CPU_UPDATE_OFFSET);
		/* hardware clears to 0; bit must be 0 before use */
	} while (reg_val);

	/* wait lock, must judge 3 times continuously */
	uint32_t judge = 0;
	do {
		reg_val = readl(reg_addr);
		reg_val &= (0x1 << PLL_CPU_LOCK_OFFSET);
		judge = (reg_val) ? judge + 1 : 0;
	} while (judge < 3);

	udelay(20);
}

static inline void sunxi_set_cpux_pll(void)
{
	/* set cpu pll 600 Mhz */
	static_pll_switch_freq(SUNXI_CCU_BASE + PLL_CPU_CTRL_REG, SUNXI_C907_CLK / 24);
}

static inline void sunxi_set_riscv_clk_sel(void)
{
	uint32_t reg_val;

	/* set cpu_div factor M */
	reg_val = readl(SUNXI_CCU_BASE + RISCV_CLK_REG);
	reg_val &= ~(RISCV_CLK_REG_RISCV_DIV_CFG_CLEAR_MASK);

	/* set cpu_axi_div factor N */
	reg_val &= ~(RISCV_CLK_REG_RISCV_AXI_DIV_CFG_CLEAR_MASK);

	writel(reg_val, SUNXI_CCU_BASE + RISCV_CLK_REG);
	udelay(10);

	/* set cpu clock source to PLL_CPU */
	reg_val = readl(SUNXI_CCU_BASE + RISCV_CLK_REG);
	reg_val &= ~(RISCV_CLK_REG_RISCV_CLK_SEL_CLEAR_MASK);
	reg_val |= (RISCV_CLK_REG_RISCV_CLK_SEL_CPUPLL << RISCV_CLK_REG_RISCV_CLK_SEL_OFFSET);

	writel(reg_val, SUNXI_CCU_BASE + RISCV_CLK_REG);
	udelay(10);
}

static inline void sunxi_set_pll_periph0(void)
{
	if (readl(SUNXI_CCU_BASE + PLL_PERI_CTRL_REG) & BIT(PLL_PERI_CTRL_REG_PLL_EN_OFFSET)) {
		/* fel/brom has already enabled pll_periph0 */
		return;
	}

	/* ldo enable */
	setbits_le32(SUNXI_CCU_BASE + PLL_PERI_CTRL_REG, BIT(PLL_PERI_CTRL_REG_PLL_LDO_EN_OFFSET));

	/* lock enable */
	setbits_le32(SUNXI_CCU_BASE + PLL_PERI_CTRL_REG, BIT(PLL_PERI_CTRL_REG_LOCK_ENABLE_OFFSET));

	/* enable PLL: 600M(1X) 1200M(2x) */
	setbits_le32(SUNXI_CCU_BASE + PLL_PERI_CTRL_REG, BIT(PLL_PERI_CTRL_REG_PLL_EN_OFFSET));

	/* wait for PLL to lock */
	while (!(readl(SUNXI_CCU_BASE + PLL_PERI_CTRL_REG) & BIT(PLL_PERI_CTRL_REG_LOCK_OFFSET)))
		;
	udelay(20);

	/* lock disable */
	clrbits_le32(SUNXI_CCU_BASE + PLL_PERI_CTRL_REG, BIT(PLL_PERI_CTRL_REG_LOCK_ENABLE_OFFSET));

	/* output enable */
	setbits_le32(SUNXI_CCU_BASE + PLL_PERI_CTRL_REG, BIT(PLL_PERI_CTRL_REG_PLL_OUTPUT_GATE_OFFSET));
}

static inline void sunxi_set_ahb_sel(void)
{
	/* BROM default configures PSI/AHB at 200MHz, keep it. */
}

static inline void sunxi_set_apb_sel(void)
{
	/* BROM default configures APB0 at 100MHz, APB1 at 24MHz, keep it. */
}

static inline void sunxi_set_pll_mbus(void)
{
	uint32_t reg_val;

	/* set factor M = 1 */
	reg_val = readl(SUNXI_CCU_BASE + MBUS_CLK_REG);
	reg_val &= ~(MBUS_CLK_REG_FACTOR_M_CLEAR_MASK);
	reg_val |= (1 << MBUS_CLK_REG_FACTOR_M_OFFSET);
	writel(reg_val, SUNXI_CCU_BASE + MBUS_CLK_REG);
	udelay(10);

	/* set clock source = DDRPLL */
	reg_val = readl(SUNXI_CCU_BASE + MBUS_CLK_REG);
	reg_val &= ~(MBUS_CLK_REG_CLK_SRC_SEL_CLEAR_MASK);
	reg_val |= (MBUS_CLK_REG_CLK_SRC_SEL_DDRPLL << MBUS_CLK_REG_CLK_SRC_SEL_OFFSET);
	writel(reg_val, SUNXI_CCU_BASE + MBUS_CLK_REG);
	udelay(10);
}

static inline void sunxi_set_dma_clk(void)
{
	/* DMA deassert reset */
	setbits_le32(SUNXI_CCU_BASE + DMA_BGR_REG, DMA_BGR_REG_SGDMA_RST_DE_ASSERT << DMA_BGR_REG_SGDMA_RST_OFFSET);
	/* DMA open gating */
	setbits_le32(SUNXI_CCU_BASE + DMA_BGR_REG, DMA_BGR_REG_SGDMA_GATING_PASS << DMA_BGR_REG_SGDMA_GATING_OFFSET);
	udelay(1);
}

void sunxi_clk_init(void)
{
	sunxi_set_cpux_pll();
	sunxi_set_pll_periph0();
	sunxi_set_riscv_clk_sel();
	sunxi_set_ahb_sel();
	sunxi_set_apb_sel();
	sunxi_set_pll_mbus();
	sunxi_set_dma_clk();
}

void sunxi_clk_dump(void)
{
	uint32_t reg_val;
	uint32_t clk_src;
	uint32_t plln;
	const char *clock_str;

	reg_val = readl(SUNXI_CCU_BASE + RISCV_CLK_REG);
	clk_src = (reg_val & RISCV_CLK_REG_RISCV_CLK_SEL_CLEAR_MASK) >> RISCV_CLK_REG_RISCV_CLK_SEL_OFFSET;

	switch (clk_src) {
	case RISCV_CLK_REG_RISCV_CLK_SEL_HOSC:
		clock_str = "OSC24M";
		break;
	case RISCV_CLK_REG_RISCV_CLK_SEL_CLK32K:
		clock_str = "CLK32K";
		break;
	case RISCV_CLK_REG_RISCV_CLK_SEL_CLK1M_RC:
		clock_str = "CLK1M_RC";
		break;
	case RISCV_CLK_REG_RISCV_CLK_SEL_PERIPLL_800M:
		clock_str = "PLL_PERI_800M";
		break;
	case RISCV_CLK_REG_RISCV_CLK_SEL_PERIPLL1X:
		clock_str = "PLL_PERI_600M";
		break;
	case RISCV_CLK_REG_RISCV_CLK_SEL_CPUPLL:
		clock_str = "PLL_CPU";
		break;
	case RISCV_CLK_REG_RISCV_CLK_SEL_AUDIO1PLL_DIV2:
		clock_str = "AUDIO1PLL_DIV2";
		break;
	default:
		clock_str = "ERROR";
		break;
	}

	plln = (readl(SUNXI_CCU_BASE + PLL_CPU_CTRL_REG) & PLL_CPU_CTRL_REG_PLL_N_CLEAR_MASK) >> PLL_CPU_CTRL_REG_PLL_N_OFFSET;

	pr_debug("CLK: CPU CLK_reg=0x%08x\n", reg_val);
	pr_debug("CLK: CPU PLL=%s FREQ=%uMHz\n", clock_str, 24 * plln);
}
