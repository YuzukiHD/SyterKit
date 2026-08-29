/* SPDX-License-Identifier: GPL-2.0+ */
#define pr_fmt(fmt) "clk-sun252iw1: " fmt

/**
 * @file clk-sun252iw1.c
 * @brief Clock driver for the Allwinner sun252iw1 SoC.
 *
 * Programs the CPU PLL, the PERIPH0 PLL and the E907/C907 core clock sources
 * together with the AHB/APB dividers during early boot.
 */

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <timer.h>

#include <log.h>

#include <drivers/clk/clk.h>
#include <drivers/clk/sun252iw1/reg.h>

#define SUNXI_C907_CLK (1008)

/**
 * @brief Set the CPUX PLL to 1008 MHz.
 *
 * Disables the output gate, programs the PLL factors for 1008 MHz, then
 * enables and locks the PLL and re-enables its output.
 */
static inline void sunxi_set_cpux_pll(void)
{
	uint32_t reg_val = 0;
	/* disable pll gating */
	clrbits_le32(SUNXI_CCU_BASE + PLL_CPU_CTRL_REG, BIT(PLL_CPU_CTRL_REG_PLL_OUTPUT_GATE_OFFSET));

	/* enable PLL output */
	setbits_le32(SUNXI_CCU_BASE + PLL_CPU_CTRL_REG, BIT(PLL_CPU_CTRL_REG_PLL_LDO_EN_OFFSET));
	udelay(5);

	/* set default values */
	/* clk is CONFIG_SUNXI_C907_FREQ, PLL_OUTPUT= 24M*N/( M*P) */
	reg_val = readl(SUNXI_CCU_BASE + PLL_CPU_CTRL_REG);
	reg_val &= ~(PLL_CPU_CTRL_REG_PLL_P_CLEAR_MASK | PLL_CPU_CTRL_REG_PLL_N_CLEAR_MASK | PLL_CPU_CTRL_REG_PLL_M1_CLEAR_MASK);
	reg_val |= (SUNXI_C907_CLK / 24) << PLL_CPU_CTRL_REG_PLL_N_OFFSET;
	writel(reg_val, SUNXI_CCU_BASE + PLL_CPU_CTRL_REG);
	reg_val = readl(SUNXI_CCU_BASE + PLL_CPU_CTRL_REG);

	/* lock enable */
	setbits_le32(SUNXI_CCU_BASE + PLL_CPU_CTRL_REG, BIT(PLL_CPU_CTRL_REG_LOCK_ENABLE_OFFSET));

	/* update enable */
	setbits_le32(SUNXI_CCU_BASE + PLL_CPU_CTRL_REG, BIT(PLL_CPU_CTRL_REG_PLL_UPDATE_OFFSET));

	/* enable pll */
	setbits_le32(SUNXI_CCU_BASE + PLL_CPU_CTRL_REG, BIT(PLL_CPU_CTRL_REG_PLL_PLL_EN_OFFSET));

	/* wait PLL_CPUX stable */
	while (!(readl(SUNXI_CCU_BASE + PLL_CPU_CTRL_REG) & BIT(PLL_CPU_CTRL_REG_LOCK_OFFSET)))
		;
	udelay(20);

	/* enable pll gating */
	setbits_le32(SUNXI_CCU_BASE + PLL_CPU_CTRL_REG, BIT(PLL_CPU_CTRL_REG_PLL_OUTPUT_GATE_OFFSET));

	/* lock disable */
	clrbits_le32(SUNXI_CCU_BASE + PLL_CPU_CTRL_REG, BIT(PLL_CPU_CTRL_REG_LOCK_ENABLE_OFFSET));

	udelay(1);
}

/**
 * @brief Enable the PERIPH0 PLL.
 *
 * Enables the LDO, lock and PLL, waits for the PLL to lock, then disables the
 * lock action and enables the PLL output. If FEL has already enabled the PLL
 * this is a no-op.
 */
static inline void sunxi_set_pll_periph0(void)
{
	if (readl(SUNXI_CCU_BASE + PLL_PERI_CTRL_REG) & BIT(PLL_PERI_CTRL_REG_PLL_EN_OFFSET)) {
		/* fel has enabled pll_periph0 */
		pr_info("pll periph0 has been enabled, skip enable\n");
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

/**
 * @brief Select the clock source for the E907 RISC-V core.
 *
 * Switches the E907 core clock to the PERI 600 MHz PLL with a divide factor
 * of 1.
 */
static inline void sunxi_set_e907_sel(void)
{
	uint32_t reg_val;

	/* set and change cpu clk src to e907 */
	reg_val = readl(SUNXI_CCU_BASE + E907_CLK_REG);
	reg_val &= ~(E907_CLK_REG_E907_CLK_SEL_CLEAR_MASK | E907_CLK_REG_E907_AXI_DIV_CFG_CLEAR_MASK | E907_CLK_REG_E907_DIV_CFG_CLEAR_MASK);
	/* set default to 600MHz, can be set to 800MHz */
	reg_val |= E907_CLK_REG_E907_CLK_SEL_PERI_600M << E907_CLK_REG_E907_CLK_SEL_OFFSET;
	/* div set to 1 */
	reg_val |= 0 << E907_CLK_REG_E907_DIV_CFG_OFFSET;
	writel(reg_val, SUNXI_CCU_BASE + E907_CLK_REG);

	udelay(1);
}

/**
 * @brief Select the clock source for the C907 RISC-V core.
 *
 * Switches the C907 core clock to the CPU PLL.
 */
static inline void sunxi_set_c907_sel(void)
{
	uint32_t reg_val;

	/* set cpu clk src to PLL_CPUX */
	reg_val = readl(SUNXI_CCU_BASE + CPU_CLK_REG);
	reg_val &= ~(CPU_CLK_REG_CPU_CLK_SEL_CLEAR_MASK | CPU_CLK_REG_CPU_AXI_DIV_CFG_CLEAR_MASK);
	reg_val |= (CPU_CLK_REG_CPU_CLK_SEL_CPUPLL_P << CPU_CLK_REG_CPU_CLK_SEL_OFFSET);
	writel(reg_val, SUNXI_CCU_BASE + CPU_CLK_REG);

	udelay(1);
}

/**
 * @brief Configure the AHB bus clock.
 *
 * Sets the AHB divider to derive 200 MHz from the 600 MHz PERI PLL.
 */
static inline void sunxi_set_ahb_sel(void)
{
	/* PLL = 600M, (M - 1) = 2, N = 1, PLL_AHB = PLL / (M + 1) / N */
	/* Set AHB Clock to 200MHz */
	writel((2 << AHB_CLK_REG_FACTOR_M_OFFSET) | (AHB_CLK_REG_FACTOR_N_1 << AHB_CLK_REG_FACTOR_N_OFFSET), SUNXI_CCU_BASE + AHB_CLK_REG);
	writel((AHB_CLK_REG_CLK_SRC_SEL_PERI_600M_BUS << AHB_CLK_REG_CLK_SRC_SEL_OFFSET) | readl(SUNXI_CCU_BASE + AHB_CLK_REG), SUNXI_CCU_BASE + AHB_CLK_REG);

	udelay(1);
}

/**
 * @brief Configure the APB0 bus clock.
 *
 * Sets the APB0 divider to derive 100 MHz from the 600 MHz PERI PLL while
 * APB1 keeps its default 24 MHz OSC source.
 */
static inline void sunxi_set_apb_sel(void)
{
	/* PLL = 600M, (M - 1) = 2, N = 1, PLL_AHB = PLL / (M + 1) / N */
	/* Set APB0 Clock to 100MHz, APB1 Keep use default 24MHz OSC  */
	writel((2 << APB0_CLK_REG_FACTOR_M_OFFSET) | (APB0_CLK_REG_FACTOR_N_2 << APB0_CLK_REG_FACTOR_N_OFFSET), SUNXI_CCU_BASE + APB0_CLK_REG);
	writel((APB0_CLK_REG_CLK_SRC_SEL_PERI_600M_BUS << APB0_CLK_REG_CLK_SRC_SEL_OFFSET) | readl(SUNXI_CCU_BASE + APB0_CLK_REG), SUNXI_CCU_BASE + APB0_CLK_REG);

	udelay(1);
}

/**
 * @brief Deassert the DMA reset and open the DMA clock gate.
 *
 * Releases the DMA from reset and enables the gating clock so the DMA engine
 * can be used by the boot loader.
 */
static inline void sunxi_set_dma_clk(void)
{
	/* DMA deassert */
	writel(readl(SUNXI_CCU_BASE + DMA_BGR_REG) | (DMA_BGR_REG_SGDMA_RST_DE_ASSERT << DMA_BGR_REG_SGDMA_RST_OFFSET), SUNXI_CCU_BASE + DMA_BGR_REG);
	/* DMA Open GATE */
	writel(readl(SUNXI_CCU_BASE + DMA_BGR_REG) | (DMA_BGR_REG_SGDMA_GATING_PASS << DMA_BGR_REG_SGDMA_GATING_OFFSET), SUNXI_CCU_BASE + DMA_BGR_REG);
	udelay(1);
}

/**
 * @brief Assert the MBUS domain reset.
 *
 * Puts the MBUS clock domain into reset so it can be configured later.
 */
static inline void sunxi_reset_mbus_domain(void)
{
	setbits_le32(SUNXI_CCU_BASE + MBUS_CLK_REG, MBUS_CLK_REG_MBUS_RST_DE_ASSERT << MBUS_CLK_REG_MBUS_RST_OFFSET);
	udelay(1);
}

#define SUNXI_MODULE_PLL_CTRL_REG_PLL_EN_OFFSET (31)
#define SUNXI_MODULE_PLL_CTRL_REG_PLL_LDO_EN_OFFSET (30)
#define SUNXI_MODULE_PLL_CTRL_REG_PLL_LOCK_ENABLE_OFFSET (29)
#define SUNXI_MODULE_PLL_CTRL_REG_PLL_LOCK_OFFSET (29)

/**
 * @brief Enable a module PLL.
 *
 * Enables the PLL, LDO and lock action for the PLL at the given register
 * offset and waits for it to lock. PLLs that are already enabled are skipped.
 *
 * @param[in] reg_offset Offset of the module PLL control register.
 */
static inline void sunxi_set_module_pll(uint32_t reg_offset)
{
	uint32_t reg_val = readl(SUNXI_CCU_BASE + reg_offset);

	/* We only enable module which not enabled */
	if (!(reg_val & BIT(SUNXI_MODULE_PLL_CTRL_REG_PLL_EN_OFFSET))) {
		/* enable pll */
		setbits_le32(SUNXI_CCU_BASE + reg_offset, BIT(SUNXI_MODULE_PLL_CTRL_REG_PLL_EN_OFFSET) | BIT(SUNXI_MODULE_PLL_CTRL_REG_PLL_LDO_EN_OFFSET));

		/* lock enable */
		setbits_le32(SUNXI_CCU_BASE + reg_offset, BIT(SUNXI_MODULE_PLL_CTRL_REG_PLL_LOCK_ENABLE_OFFSET));

		/* wait pll lock */
		while (!(readl(SUNXI_CCU_BASE + reg_offset) & BIT(SUNXI_MODULE_PLL_CTRL_REG_PLL_LOCK_OFFSET)))
			;
		udelay(20);

		/* lock done, disable lock action */
		clrbits_le32(SUNXI_CCU_BASE + reg_offset, BIT(SUNXI_MODULE_PLL_CTRL_REG_PLL_LOCK_ENABLE_OFFSET));
	}
}

/**
 * @brief Initialize the SoC clocks.
 *
 * Programs the CPUX and PERIPH0 PLLs, selects the E907/C907 core clocks,
 * configures the AHB/APB dividers, DMA and MBUS clocks, and enables the
 * module PLLs.
 */
void sunxi_clk_init(void)
{
	sunxi_set_cpux_pll();
	sunxi_set_pll_periph0();
	sunxi_set_e907_sel();
	sunxi_set_c907_sel();
	sunxi_set_ahb_sel();
	sunxi_set_apb_sel();
	sunxi_set_dma_clk();
	sunxi_reset_mbus_domain();
	sunxi_set_module_pll(PLL_PERI_CTRL_REG);
	sunxi_set_module_pll(PLL_VIDEO_CTRL_REG);
	sunxi_set_module_pll(PLL_AUDIO_CTRL_REG);
}

/**
 * @brief Dump the current CPU clock configuration.
 *
 * Prints the CPU clock source and the computed CPU frequency.
 */
void sunxi_clk_dump(void)
{
	uint32_t reg_val;
	uint32_t cpu_clk_src, clk_freq, plln, pllm;
	uint8_t p0, p1;
	const char *clock_str;

	/* PLL CPU */
	reg_val = readl(SUNXI_CCU_BASE + CPU_CLK_REG);
	cpu_clk_src = (reg_val & CPU_CLK_REG_CPU_CLK_SEL_CLEAR_MASK) >> CPU_CLK_REG_CPU_CLK_SEL_OFFSET;
	pr_debug("CPU CLK_reg=0x%08x\n", reg_val);

	switch (cpu_clk_src) {
	case CPU_CLK_REG_CPU_CLK_SEL_HOSC:
		clock_str = "OSC24M";
		break;

	case CPU_CLK_REG_CPU_CLK_SEL_CLK32K:
		clock_str = "CLK32";
		break;

	case CPU_CLK_REG_CPU_CLK_SEL_CLK16M_RC:
		clock_str = "CLK16M_RC";
		break;

	case CPU_CLK_REG_CPU_CLK_SEL_CPUPLL_P:
		clock_str = "PLL_CPU";
		break;

	case CPU_CLK_REG_CPU_CLK_SEL_PERI_600M_BUS:
		clock_str = "PLL_PERI_600M";
		break;

	case CPU_CLK_REG_CPU_CLK_SEL_PERI_800M:
		clock_str = "PLL_PERI_800M";
		break;

	default:
		clock_str = "ERROR";
	}

	p0 = (reg_val & CPU_CLK_REG_PLL_CPU_OUT_EXT_DIVP_CLEAR_MASK) >> CPU_CLK_REG_PLL_CPU_OUT_EXT_DIVP_OFFSET;
	if (p0 == CPU_CLK_REG_PLL_CPU_OUT_EXT_DIVP_1) {
		p1 = 1;
	} else if (p0 == CPU_CLK_REG_PLL_CPU_OUT_EXT_DIVP_2) {
		p1 = 2;
	} else if (p0 == CPU_CLK_REG_PLL_CPU_OUT_EXT_DIVP_4) {
		p1 = 4;
	} else {
		p1 = 1;
	}

	plln = (readl(SUNXI_CCU_BASE + PLL_CPU_CTRL_REG) & PLL_CPU_CTRL_REG_PLL_N_CLEAR_MASK) >> PLL_CPU_CTRL_REG_PLL_N_OFFSET;
	pllm = (readl(SUNXI_CCU_BASE + PLL_CPU_CTRL_REG) & PLL_CPU_CTRL_REG_PLL_M1_CLEAR_MASK) >> PLL_CPU_CTRL_REG_PLL_M1_OFFSET;
	clk_freq = 24 * (pllm + 1) * plln / p1;

	pr_debug("CPU PLL=%s FREQ=%uMHz\n", clock_str, clk_freq);
}
