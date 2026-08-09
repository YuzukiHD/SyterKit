/* SPDX-License-Identifier: GPL-2.0+ */

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <timer.h>

#include <log.h>

#include <drivers/clk.h>
#include <drivers/reg/reg-ccu.h>
#include <dt2c/driver.h>

#define SUNXI_C907_CLK (1008)

static inline void sunxi_set_cpux_pll(sunxi_ccu_t *ccu) {
	uint32_t reg_val = 0;
	/* disable pll gating */
	clrbits_le32(sunxi_ccu_reg(ccu, PLL_CPU_CTRL_REG), BIT(PLL_CPU_CTRL_REG_PLL_OUTPUT_GATE_OFFSET));

	/* enable PLL output */
	setbits_le32(sunxi_ccu_reg(ccu, PLL_CPU_CTRL_REG), BIT(PLL_CPU_CTRL_REG_PLL_LDO_EN_OFFSET));
	udelay(5);

	/* set default values */
	/* clk is CONFIG_SUNXI_C907_FREQ, PLL_OUTPUT= 24M*N/( M*P) */
	reg_val = readl(sunxi_ccu_reg(ccu, PLL_CPU_CTRL_REG));
	reg_val &= ~(PLL_CPU_CTRL_REG_PLL_P_CLEAR_MASK | PLL_CPU_CTRL_REG_PLL_N_CLEAR_MASK | PLL_CPU_CTRL_REG_PLL_M1_CLEAR_MASK);
	reg_val |= (SUNXI_C907_CLK / 24) << PLL_CPU_CTRL_REG_PLL_N_OFFSET;
	writel(reg_val, sunxi_ccu_reg(ccu, PLL_CPU_CTRL_REG));
	reg_val = readl(sunxi_ccu_reg(ccu, PLL_CPU_CTRL_REG));

	/* lock enable */
	setbits_le32(sunxi_ccu_reg(ccu, PLL_CPU_CTRL_REG), BIT(PLL_CPU_CTRL_REG_LOCK_ENABLE_OFFSET));

	/* update enable */
	setbits_le32(sunxi_ccu_reg(ccu, PLL_CPU_CTRL_REG), BIT(PLL_CPU_CTRL_REG_PLL_UPDATE_OFFSET));

	/* enable pll */
	setbits_le32(sunxi_ccu_reg(ccu, PLL_CPU_CTRL_REG), BIT(PLL_CPU_CTRL_REG_PLL_PLL_EN_OFFSET));

	/* wait PLL_CPUX stable */
	while (!(readl(sunxi_ccu_reg(ccu, PLL_CPU_CTRL_REG)) & BIT(PLL_CPU_CTRL_REG_LOCK_OFFSET)))
		;
	udelay(20);

	/* enable pll gating */
	setbits_le32(sunxi_ccu_reg(ccu, PLL_CPU_CTRL_REG), BIT(PLL_CPU_CTRL_REG_PLL_OUTPUT_GATE_OFFSET));

	/* lock disable */
	clrbits_le32(sunxi_ccu_reg(ccu, PLL_CPU_CTRL_REG), BIT(PLL_CPU_CTRL_REG_LOCK_ENABLE_OFFSET));

	udelay(1);
}

static inline void sunxi_set_pll_periph0(sunxi_ccu_t *ccu) {
	if (readl(sunxi_ccu_reg(ccu, PLL_PERI_CTRL_REG)) & BIT(PLL_PERI_CTRL_REG_PLL_EN_OFFSET)) {
		/* fel has enabled pll_periph0 */
		printk_info("pll periph0 has been enabled, skip enable\n");
		return;
	}

	/* ldo enable */
	setbits_le32(sunxi_ccu_reg(ccu, PLL_PERI_CTRL_REG), BIT(PLL_PERI_CTRL_REG_PLL_LDO_EN_OFFSET));

	/* lock enable */
	setbits_le32(sunxi_ccu_reg(ccu, PLL_PERI_CTRL_REG), BIT(PLL_PERI_CTRL_REG_LOCK_ENABLE_OFFSET));

	/* enable PLL: 600M(1X) 1200M(2x) */
	setbits_le32(sunxi_ccu_reg(ccu, PLL_PERI_CTRL_REG), BIT(PLL_PERI_CTRL_REG_PLL_EN_OFFSET));

	/* wait for PLL to lock */
	while (!(readl(sunxi_ccu_reg(ccu, PLL_PERI_CTRL_REG)) & BIT(PLL_PERI_CTRL_REG_LOCK_OFFSET)))
		;
	udelay(20);

	/* lock disable */
	clrbits_le32(sunxi_ccu_reg(ccu, PLL_PERI_CTRL_REG), BIT(PLL_PERI_CTRL_REG_LOCK_ENABLE_OFFSET));

	/* output enable */
	setbits_le32(sunxi_ccu_reg(ccu, PLL_PERI_CTRL_REG), BIT(PLL_PERI_CTRL_REG_PLL_OUTPUT_GATE_OFFSET));
}

static inline void sunxi_set_e907_sel(sunxi_ccu_t *ccu) {
	uint32_t reg_val;

	/* set and change cpu clk src to e907 */
	reg_val = readl(sunxi_ccu_reg(ccu, E907_CLK_REG));
	reg_val &= ~(E907_CLK_REG_E907_CLK_SEL_CLEAR_MASK |
				 E907_CLK_REG_E907_AXI_DIV_CFG_CLEAR_MASK |
				 E907_CLK_REG_E907_DIV_CFG_CLEAR_MASK);
	/* set default to 600MHz, can be set to 800MHz */
	reg_val |= E907_CLK_REG_E907_CLK_SEL_PERI_600M << E907_CLK_REG_E907_CLK_SEL_OFFSET;
	/* div set to 1 */
	reg_val |= 0 << E907_CLK_REG_E907_DIV_CFG_OFFSET;
	writel(reg_val, sunxi_ccu_reg(ccu, E907_CLK_REG));

	udelay(1);
}

static inline void sunxi_set_c907_sel(sunxi_ccu_t *ccu) {
	uint32_t reg_val;

	/* set cpu clk src to PLL_CPUX */
	reg_val = readl(sunxi_ccu_reg(ccu, CPU_CLK_REG));
	reg_val &= ~(CPU_CLK_REG_CPU_CLK_SEL_CLEAR_MASK | CPU_CLK_REG_CPU_AXI_DIV_CFG_CLEAR_MASK);
	reg_val |= (CPU_CLK_REG_CPU_CLK_SEL_CPUPLL_P << CPU_CLK_REG_CPU_CLK_SEL_OFFSET);
	writel(reg_val, sunxi_ccu_reg(ccu, CPU_CLK_REG));

	udelay(1);
}

static inline void sunxi_set_ahb_sel(sunxi_ccu_t *ccu) {
	/* PLL = 600M, (M - 1) = 2, N = 1, PLL_AHB = PLL / (M + 1) / N */
	/* Set AHB Clock to 200MHz */
	writel((2 << AHB_CLK_REG_FACTOR_M_OFFSET) | (AHB_CLK_REG_FACTOR_N_1 << AHB_CLK_REG_FACTOR_N_OFFSET),
		   sunxi_ccu_reg(ccu, AHB_CLK_REG));
	writel((AHB_CLK_REG_CLK_SRC_SEL_PERI_600M_BUS << AHB_CLK_REG_CLK_SRC_SEL_OFFSET) |
				   readl(sunxi_ccu_reg(ccu, AHB_CLK_REG)),
		   sunxi_ccu_reg(ccu, AHB_CLK_REG));

	udelay(1);
}

static inline void sunxi_set_apb_sel(sunxi_ccu_t *ccu) {
	/* PLL = 600M, (M - 1) = 2, N = 1, PLL_AHB = PLL / (M + 1) / N */
	/* Set APB0 Clock to 100MHz, APB1 Keep use default 24MHz OSC  */
	writel((2 << APB0_CLK_REG_FACTOR_M_OFFSET) | (APB0_CLK_REG_FACTOR_N_2 << APB0_CLK_REG_FACTOR_N_OFFSET),
		   sunxi_ccu_reg(ccu, APB0_CLK_REG));
	writel((APB0_CLK_REG_CLK_SRC_SEL_PERI_600M_BUS << APB0_CLK_REG_CLK_SRC_SEL_OFFSET) |
				   readl(sunxi_ccu_reg(ccu, APB0_CLK_REG)),
		   sunxi_ccu_reg(ccu, APB0_CLK_REG));

	udelay(1);
}

static inline void sunxi_set_dma_clk(sunxi_ccu_t *ccu) {
	/* DMA deassert */
	writel(readl(sunxi_ccu_reg(ccu, DMA_BGR_REG)) |
				   (DMA_BGR_REG_SGDMA_RST_DE_ASSERT << DMA_BGR_REG_SGDMA_RST_OFFSET),
		   sunxi_ccu_reg(ccu, DMA_BGR_REG));
	/* DMA Open GATE */
	writel(readl(sunxi_ccu_reg(ccu, DMA_BGR_REG)) |
				   (DMA_BGR_REG_SGDMA_GATING_PASS << DMA_BGR_REG_SGDMA_GATING_OFFSET),
		   sunxi_ccu_reg(ccu, DMA_BGR_REG));
	udelay(1);
}

static inline void sunxi_reset_mbus_domain(sunxi_ccu_t *ccu) {
	setbits_le32(sunxi_ccu_reg(ccu, MBUS_CLK_REG), MBUS_CLK_REG_MBUS_RST_DE_ASSERT << MBUS_CLK_REG_MBUS_RST_OFFSET);
	udelay(1);
}


#define SUNXI_MODULE_PLL_CTRL_REG_PLL_EN_OFFSET (31)
#define SUNXI_MODULE_PLL_CTRL_REG_PLL_LDO_EN_OFFSET (30)
#define SUNXI_MODULE_PLL_CTRL_REG_PLL_LOCK_ENABLE_OFFSET (29)
#define SUNXI_MODULE_PLL_CTRL_REG_PLL_LOCK_OFFSET (29)

static inline void sunxi_set_module_pll(sunxi_ccu_t *ccu,
					uint32_t reg_offset) {
	uint32_t reg_val = readl(sunxi_ccu_reg(ccu, reg_offset));

	/* We only enable module which not enabled */
	if (!(reg_val & BIT(SUNXI_MODULE_PLL_CTRL_REG_PLL_EN_OFFSET))) {
		/* enable pll */
		setbits_le32(sunxi_ccu_reg(ccu, reg_offset), BIT(SUNXI_MODULE_PLL_CTRL_REG_PLL_EN_OFFSET) |
														BIT(SUNXI_MODULE_PLL_CTRL_REG_PLL_LDO_EN_OFFSET));

		/* lock enable */
		setbits_le32(sunxi_ccu_reg(ccu, reg_offset), BIT(SUNXI_MODULE_PLL_CTRL_REG_PLL_LOCK_ENABLE_OFFSET));

		/* wait pll lock */
		while (!(readl(sunxi_ccu_reg(ccu, reg_offset)) & BIT(SUNXI_MODULE_PLL_CTRL_REG_PLL_LOCK_OFFSET)))
			;
		udelay(20);

		/* lock done, disable lock action */
		clrbits_le32(sunxi_ccu_reg(ccu, reg_offset), BIT(SUNXI_MODULE_PLL_CTRL_REG_PLL_LOCK_ENABLE_OFFSET));
	}
}

void sunxi_clk_init(sunxi_ccu_t *ccu) {
	sunxi_set_cpux_pll(ccu);
	sunxi_set_pll_periph0(ccu);
	sunxi_set_e907_sel(ccu);
	sunxi_set_c907_sel(ccu);
	sunxi_set_ahb_sel(ccu);
	sunxi_set_apb_sel(ccu);
	sunxi_set_dma_clk(ccu);
	sunxi_reset_mbus_domain(ccu);
	sunxi_set_module_pll(ccu, PLL_PERI_CTRL_REG);
	sunxi_set_module_pll(ccu, PLL_VIDEO_CTRL_REG);
	sunxi_set_module_pll(ccu, PLL_AUDIO_CTRL_REG);
}

void sunxi_clk_dump(sunxi_ccu_t *ccu) {
	uint32_t reg_val;
	uint32_t cpu_clk_src, clk_freq, plln, pllm;
	uint8_t p0, p1;
	const char *clock_str;

	/* PLL CPU */
	reg_val = readl(sunxi_ccu_reg(ccu, CPU_CLK_REG));
	cpu_clk_src = (reg_val & CPU_CLK_REG_CPU_CLK_SEL_CLEAR_MASK) >> CPU_CLK_REG_CPU_CLK_SEL_OFFSET;
	printk_debug("CLK: CPU CLK_reg=0x%08x\n", reg_val);

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

	plln = (readl(sunxi_ccu_reg(ccu, PLL_CPU_CTRL_REG)) & PLL_CPU_CTRL_REG_PLL_N_CLEAR_MASK) >> PLL_CPU_CTRL_REG_PLL_N_OFFSET;
	pllm = (readl(sunxi_ccu_reg(ccu, PLL_CPU_CTRL_REG)) & PLL_CPU_CTRL_REG_PLL_M1_CLEAR_MASK) >> PLL_CPU_CTRL_REG_PLL_M1_OFFSET;
	clk_freq = 24 * (pllm + 1) * plln / p1;

	printk_debug("CLK: CPU PLL=%s FREQ=%uMHz\n", clock_str, clk_freq);
}

/* we got hosc freq in arch/timer.c */
extern uint32_t current_hosc_freq;

uint32_t sunxi_clk_get_peri1x_rate(sunxi_ccu_t *ccu) {
	(void) ccu;
	return 192; /* PERI_192M */
}

DT2C_DRIVER_COMPAT("allwinner,sun252iw1-ccu");
