/* SPDX-License-Identifier: GPL-2.0+ */

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <timer.h>

#include <log.h>

#include <sys-clk.h>

static void set_pll_general(uint32_t pll_addr, uint32_t en, uint32_t output_gate_en, uint32_t pll_d, uint32_t pll_d_off, uint32_t pll_n) {
	uint32_t pll_en;
	uint32_t pll_ldo_en;
	uint32_t pll_lock_en;
	uint32_t pll_output_gate;
	uint32_t pll_m;

	if (en == 1) {
		pll_en = PLL_Enable;
		pll_ldo_en = PLL_LDO_Enable;
		pll_lock_en = PLL_LOCK_EN_Enable;
	} else {
		pll_en = PLL_Disable;
		pll_ldo_en = PLL_LDO_Disable;
		pll_lock_en = PLL_LOCK_EN_Disable;
	}

	pll_output_gate = PLL_OUTPUT_GATE_Disable;
	clrsetbits_le32(pll_addr, PLL_D_MASK, pll_d << pll_d_off);
	clrsetbits_le32(pll_addr, PLL_N_MASK, pll_n << PLL_N_OFFSET);
	clrsetbits_le32(pll_addr, PLL_LDO_MASK | PLL_OUTPUT_GATE_MASK | PLL_EN_MASK, pll_en | pll_ldo_en | pll_lock_en);
	clrsetbits_le32(pll_addr, PLL_LOCK_EN_MASK, PLL_LOCK_EN_Enable);

	while (!(readl(pll_addr) & PLL_LOCK_MASK))
		;

	clrsetbits_le32(pll_addr, PLL_OUTPUT_GATE_MASK, PLL_OUTPUT_GATE_Enable);

	return;
}

static void set_pll_e90x(void) {
	clrsetbits_le32(CCU_E90X_CLK_REG, E907_CLK_REG_E907_CLK_DIV_CLEAR_MASK, CCU_E90X_CLK_CPU_M_1 << E907_CLK_REG_E907_CLK_DIV_OFFSET);
	clrsetbits_le32(CCU_E90X_CLK_REG, E907_CLK_REG_E907_CLK_SEL_CLEAR_MASK, E907_CLK_REG_E907_CLK_SEL_PERI_PLL_614M << E907_CLK_REG_E907_CLK_SEL_OFFSET);
	return;
}

static void set_pll_a27l2(void) {
	clrsetbits_le32(CCU_A27_CLK_REG, A27L2_CLK_REG_A27L2_CLK_DIV_CLEAR_MASK, CCU_A27_CLK_CPU_M_1 << A27L2_CLK_REG_A27L2_CLK_DIV_OFFSET);
	clrsetbits_le32(CCU_A27_CLK_REG, A27L2_CLK_REG_A27L2_CLK_SEL_CLEAR_MASK, A27L2_CLK_REG_A27L2_CLK_SEL_CPU_PLL << A27L2_CLK_REG_A27L2_CLK_SEL_OFFSET);
	clrsetbits_le32(CCU_A27_CLK_REG, A27L2_CLK_REG_A27L2_CLK_EN_CLEAR_MASK, A27L2_CLK_REG_A27L2_CLK_EN_CLOCK_IS_ON << A27L2_CLK_REG_A27L2_CLK_EN_OFFSET);
	return;
}

static void set_pll_peri_ctrl0(uint32_t en, uint32_t output_gate_en, uint32_t pll_n, uint32_t pll_m) {
	uint32_t pll_en;
	uint32_t pll_ldo_en;
	uint32_t pll_lock_en;
	uint32_t pll_output_gate;

	if (en == 1) {
		pll_en = PLL_PERI_CTRL0_REG_PLL_EN_ENABLE << PLL_PERI_CTRL0_REG_PLL_EN_OFFSET;
		pll_ldo_en = PLL_PERI_CTRL0_REG_PLL_LDO_EN_ENABLE << PLL_PERI_CTRL0_REG_PLL_LDO_EN_OFFSET;
		pll_lock_en = PLL_PERI_CTRL0_REG_LOCK_ENABLE_ENABLE << PLL_PERI_CTRL0_REG_LOCK_ENABLE_OFFSET;
	} else {
		pll_en = PLL_PERI_CTRL0_REG_PLL_EN_DISABLE << PLL_PERI_CTRL0_REG_PLL_EN_OFFSET;
		pll_ldo_en = PLL_PERI_CTRL0_REG_PLL_LDO_EN_DISABLE << PLL_PERI_CTRL0_REG_PLL_LDO_EN_OFFSET;
		pll_lock_en = PLL_PERI_CTRL0_REG_LOCK_ENABLE_DISABLE << PLL_PERI_CTRL0_REG_LOCK_ENABLE_OFFSET;
	}
	pll_output_gate = PLL_VIDEO_CTRL_REG_PLL_OUTPUT_GATE_DISABLE << PLL_VIDEO_CTRL_REG_PLL_OUTPUT_GATE_OFFSET;
	pll_n = pll_n << PLL_PERI_CTRL0_REG_PLL_N_OFFSET;
	pll_m = pll_m << PLL_PERI_CTRL0_REG_PLL_INPUT_DIV_OFFSET;

	clrsetbits_le32(CCU_PLL_PERI_CTRL0_REG, PLL_PERI_CTRL0_REG_PLL_INPUT_DIV_CLEAR_MASK, pll_m);
	clrsetbits_le32(CCU_PLL_PERI_CTRL0_REG, PLL_PERI_CTRL0_REG_PLL_N_CLEAR_MASK, pll_n);

	clrsetbits_le32(CCU_PLL_PERI_CTRL0_REG, PLL_PERI_CTRL0_REG_PLL_EN_CLEAR_MASK | PLL_PERI_CTRL0_REG_PLL_LDO_EN_CLEAR_MASK | PLL_PERI_CTRL0_REG_PLL_OUTPUT_GATE_CLEAR_MASK,
					pll_en | pll_ldo_en | pll_output_gate);

	clrsetbits_le32(CCU_PLL_PERI_CTRL0_REG, PLL_PERI_CTRL0_REG_LOCK_ENABLE_CLEAR_MASK, pll_lock_en);

	while (!(readl(CCU_PLL_PERI_CTRL0_REG) & PLL_PERI_CTRL0_REG_LOCK_CLEAR_MASK))
		;

	if (output_gate_en == 1) {
		pll_output_gate = PLL_PERI_CTRL0_REG_PLL_OUTPUT_GATE_ENABLE << PLL_PERI_CTRL0_REG_PLL_OUTPUT_GATE_OFFSET;
		clrsetbits_le32(CCU_PLL_PERI_CTRL0_REG, PLL_PERI_CTRL0_REG_PLL_OUTPUT_GATE_CLEAR_MASK, pll_output_gate);
	}
}

static void set_pll_peri_ctrl1(void) {
	uint32_t reg;
	reg = readl(CCU_PLL_PERI_CTRL1_REG);
	// reg |= PLL_PERI_CTRL1_REG_PLL_PERI_CKO_192_EN_ENABLE
	//        << PLL_PERI_CTRL1_REG_PLL_PERI_CKO_192_EN_OFFSET;
	reg = 0xFFFF;
	writel(reg, CCU_PLL_PERI_CTRL1_REG);
	return;
}

/* pll peri hosc*2N/M = 3072M  hardware * 2 */
static void set_pll_peri(void) {
	if (!(readl(CCU_PLL_PERI_CTRL0_REG) & PLL_PERI_CTRL0_REG_PLL_EN_CLEAR_MASK)) {
		if (sunxi_clk_get_hosc_type() == HOSC_FREQ_40M) {
			set_pll_peri_ctrl0(PLL_PERI_CTRL0_REG_PLL_EN_ENABLE, PLL_PERI_CTRL0_REG_PLL_OUTPUT_GATE_ENABLE, CCU_AON_PLL_CPU_N_192, CCU_AON_PLL_CPU_M_5);
		} else {
			set_pll_peri_ctrl0(PLL_PERI_CTRL0_REG_PLL_EN_ENABLE, PLL_PERI_CTRL0_REG_PLL_OUTPUT_GATE_ENABLE, CCU_AON_PLL_CPU_N_192, CCU_AON_PLL_CPU_M_3);
		}
	}
	set_pll_peri_ctrl1();
	return;
}

/* pll csi rate = 675M = hosc / 4 * N , N = N(INT) + N(FRAC) */
static void set_pll_csi(void) {
	uint32_t n, input_div, wave_bot;

	/* Set N、M */
	if (sunxi_clk_get_hosc_type() == HOSC_FREQ_40M) {
		n = CCU_AON_PLL_CPU_N_67;
		wave_bot = 0xc0010000;
		input_div = PLL_CSI_CTRL_REG_PLL_INPUT_DIV_4;
	} else {
		n = CCU_AON_PLL_CPU_N_56;
		wave_bot = 0xc0008000;
		input_div = PLL_CSI_CTRL_REG_PLL_INPUT_DIV_2;
	}
	clrsetbits_le32(CCU_PLL_CSI_CTRL_REG, PLL_CSI_CTRL_REG_PLL_FACTOR_N_CLEAR_MASK, n << PLL_CSI_CTRL_REG_PLL_FACTOR_N_OFFSET);
	clrsetbits_le32(CCU_PLL_CSI_CTRL_REG, PLL_CSI_CTRL_REG_PLL_INPUT_DIV_CLEAR_MASK, input_div << PLL_CSI_CTRL_REG_PLL_INPUT_DIV_OFFSET);

	/* Enable PLL_EN PLL_LEO_EN */
	clrsetbits_le32(CCU_PLL_CSI_CTRL_REG, PLL_CSI_CTRL_REG_PLL_EN_CLEAR_MASK, PLL_CSI_CTRL_REG_PLL_EN_ENABLE << PLL_CSI_CTRL_REG_PLL_EN_OFFSET);
	clrsetbits_le32(CCU_PLL_PERI_CTRL0_REG, PLL_CSI_CTRL_REG_PLL_LDO_EN_CLEAR_MASK, PLL_CSI_CTRL_REG_PLL_LDO_EN_ENABLE << PLL_CSI_CTRL_REG_PLL_LDO_EN_OFFSET);

	/* Enable SDM */
	clrsetbits_le32(CCU_PLL_CSI_CTRL_REG, PLL_CSI_CTRL_REG_PLL_SDM_EN_CLEAR_MASK, PLL_CSI_CTRL_REG_PLL_SDM_EN_ENABLE << PLL_CSI_CTRL_REG_PLL_SDM_EN_OFFSET);

	/* Set WAVE_BOT and SPR_FREQ_MODE of PAT0 */
	clrsetbits_le32(CCU_PLL_CSI_PAT0_REG, PLL_CSI_PAT0_CTRL_REG_WAVE_BOT_CLEAR_MASK, wave_bot << PLL_CSI_PAT0_CTRL_REG_WAVE_BOT_OFFSET);

	/* Enable SIG_DELT_PAT_EN of PAT1 */
	clrsetbits_le32(CCU_PLL_CSI_PAT1_REG, PLL_CSI_PAT1_CTRL_REG_SIG_DELT_PAT_EN_CLEAR_MASK, 0x1 << PLL_CSI_PAT1_CTRL_REG_SIG_DELT_PAT_EN_OFFSET);

	/* Disable OUTPUT and Enable LOCK_ENABLE */
	clrsetbits_le32(CCU_PLL_CSI_CTRL_REG, PLL_CSI_CTRL_REG_PLL_OUTPUT_GATE_CLEAR_MASK, PLL_CSI_CTRL_REG_PLL_OUTPUT_GATE_DISABLE << PLL_CSI_CTRL_REG_PLL_OUTPUT_GATE_OFFSET);
	clrsetbits_le32(CCU_PLL_CSI_CTRL_REG, PLL_CSI_CTRL_REG_LOCK_ENABLE_CLEAR_MASK, PLL_CSI_CTRL_REG_LOCK_ENABLE_ENABLE << PLL_CSI_CTRL_REG_LOCK_ENABLE_OFFSET);

	/* Wait Lock_status */
	while (!(readl(CCU_PLL_CSI_CTRL_REG) & PLL_CSI_CTRL_REG_LOCK_CLEAR_MASK))
		;

	/* Enable Output */
	clrsetbits_le32(CCU_PLL_CSI_CTRL_REG, PLL_CSI_CTRL_REG_PLL_OUTPUT_GATE_CLEAR_MASK, PLL_CSI_CTRL_REG_PLL_OUTPUT_GATE_ENABLE << PLL_CSI_CTRL_REG_PLL_OUTPUT_GATE_OFFSET);
}

/* pll video = 1200M = hosc * N */
static void set_pll_video(void) {
	/* Close Lock_en and Output_gate */
	clrsetbits_le32(CCU_PLL_VIDEO_CTRL_REG, PLL_VIDEO_CTRL_REG_LOCK_ENABLE_CLEAR_MASK, PLL_VIDEO_CTRL_REG_LOCK_ENABLE_DISABLE << PLL_VIDEO_CTRL_REG_LOCK_ENABLE_OFFSET);
	clrsetbits_le32(CCU_PLL_VIDEO_CTRL_REG, PLL_VIDEO_CTRL_REG_PLL_OUTPUT_GATE_CLEAR_MASK, PLL_VIDEO_CTRL_REG_PLL_OUTPUT_GATE_DISABLE << PLL_VIDEO_CTRL_REG_PLL_OUTPUT_GATE_OFFSET);

	/* Set N、M */
	if (sunxi_clk_get_hosc_type() == HOSC_FREQ_40M) {
		clrsetbits_le32(CCU_PLL_VIDEO_CTRL_REG, PLL_VIDEO_CTRL_REG_PLL_N_CLEAR_MASK, CCU_AON_PLL_CPU_N_30 << PLL_VIDEO_CTRL_REG_PLL_N_OFFSET);
	} else {
		clrsetbits_le32(CCU_PLL_VIDEO_CTRL_REG, PLL_VIDEO_CTRL_REG_PLL_N_CLEAR_MASK, CCU_AON_PLL_CPU_N_50 << PLL_VIDEO_CTRL_REG_PLL_N_OFFSET);
	}
	clrsetbits_le32(CCU_PLL_VIDEO_CTRL_REG, PLL_VIDEO_CTRL_REG_PLL_INPUT_DIV_CLEAR_MASK, PLL_VIDEO_CTRL_REG_PLL_INPUT_DIV_1 << PLL_VIDEO_CTRL_REG_PLL_INPUT_DIV_OFFSET);

	/* Enable Lock_en */
	clrsetbits_le32(CCU_PLL_VIDEO_CTRL_REG, PLL_VIDEO_CTRL_REG_LOCK_ENABLE_CLEAR_MASK, PLL_VIDEO_CTRL_REG_LOCK_ENABLE_ENABLE << PLL_VIDEO_CTRL_REG_LOCK_ENABLE_OFFSET);

	/* Wait Lock_status */
	while (!(readl(CCU_PLL_VIDEO_CTRL_REG) & PLL_VIDEO_CTRL_REG_LOCK_CLEAR_MASK))
		;

	/* Enable Output */
	clrsetbits_le32(CCU_PLL_VIDEO_CTRL_REG, PLL_VIDEO_CTRL_REG_PLL_OUTPUT_GATE_CLEAR_MASK, PLL_VIDEO_CTRL_REG_PLL_OUTPUT_GATE_ENABLE << PLL_VIDEO_CTRL_REG_PLL_OUTPUT_GATE_OFFSET);
}

// ahb = 768/4 = 192M
static void set_ahb(void) {
	clrsetbits_le32(CCU_AHB_CLK_REG, AHB_CLK_REG_AHB_CLK_DIV_CLEAR_MASK, CCU_AON_PLL_CPU_M_4 << AHB_CLK_REG_AHB_CLK_DIV_OFFSET);
	udelay(2);
	clrsetbits_le32(CCU_AHB_CLK_REG, AHB_CLK_REG_AHB_SEL_CLEAR_MASK, AHB_CLK_REG_AHB_SEL_PERI_768M << AHB_CLK_REG_AHB_SEL_OFFSET);
	udelay(2);
	return;
}

// apb = 384/4 = 96M
static void set_apb(void) {
	clrsetbits_le32(CCU_APB_CLK_REG, APB_CLK_REG_APB_CLK_DIV_CLEAR_MASK, CCU_AON_PLL_CPU_M_4 << APB_CLK_REG_APB_CLK_DIV_OFFSET);
	udelay(2);
	clrsetbits_le32(CCU_APB_CLK_REG, APB_CLK_REG_APB_SEL_CLEAR_MASK, APB_CLK_REG_APB_SEL_PERI_384M << APB_CLK_REG_APB_SEL_OFFSET);
	udelay(2);
	return;
}

// 192M
static void set_apb_spec(void) {
	clrsetbits_le32(CCU_APB_SPEC_CLK_REG, APB_SPEC_CLK_REG_APB_SPEC_SEL_CLEAR_MASK | APB_SPEC_CLK_REG_APB_SPEC_CLK_DIV_CLEAR_MASK,
					APB_SPEC_CLK_REG_APB_SPEC_SEL_PERI_192M << APB_SPEC_CLK_REG_APB_SPEC_SEL_OFFSET);
	return;
}

void sunxi_clk_init(void) {
	/* detect hosc */
	if (sunxi_clk_get_hosc_type() == HOSC_FREQ_40M) {
		set_pll_general(CCU_PLL_CPUX_CTRL_REG, PLL_CPU_CTRL_REG_PLL_EN_ENABLE, PLL_CPU_CTRL_REG_PLL_OUTPUT_GATE_ENABLE, CCU_AON_PLL_CPU_D_1, 2, CCU_AON_PLL_CPU_N_27);
		if (!(readl(CCU_PLL_VIDEO_CTRL_REG) & PLL_CPU_CTRL_REG_PLL_EN_CLEAR_MASK)) {
			set_pll_general(CCU_PLL_VIDEO_CTRL_REG, PLL_CPU_CTRL_REG_PLL_EN_ENABLE, PLL_CPU_CTRL_REG_PLL_OUTPUT_GATE_ENABLE, CCU_AON_PLL_CPU_D_4, 1, CCU_AON_PLL_CPU_N_118);
		}
	} else {
		set_pll_general(CCU_PLL_CPUX_CTRL_REG, PLL_CPU_CTRL_REG_PLL_EN_ENABLE, PLL_CPU_CTRL_REG_PLL_OUTPUT_GATE_ENABLE, CCU_AON_PLL_CPU_D_1, 2, CCU_AON_PLL_CPU_N_45);
		if (!(readl(CCU_PLL_VIDEO_CTRL_REG) & PLL_CPU_CTRL_REG_PLL_EN_CLEAR_MASK)) {
			set_pll_general(CCU_PLL_VIDEO_CTRL_REG, PLL_CPU_CTRL_REG_PLL_EN_ENABLE, PLL_CPU_CTRL_REG_PLL_OUTPUT_GATE_ENABLE, CCU_AON_PLL_CPU_D_2, 1, CCU_AON_PLL_CPU_N_99);
		}
	}

	set_pll_e90x();
	set_pll_a27l2();
	set_ahb();
	set_apb();
	set_pll_video();
	set_pll_csi();
}

void sunxi_clk_pre_init(void) {
	set_apb_spec();
	set_pll_peri();
}

void sunxi_clk_dump() {
	uint32_t reg_val = 0;
	uint32_t n, m, clock_src, clock, vd_reg, pll_cpu_reg;

	printk_debug("SoC HOSC Type = %d MHz\n", sunxi_clk_get_hosc_type());

	reg_val = readl(SUNXI_CCM_AON_BASE + E907_CLK_REG);
	clock_src = ((reg_val & E907_CLK_REG_E907_CLK_SEL_CLEAR_MASK) >> E907_CLK_REG_E907_CLK_SEL_OFFSET);
	switch (clock_src) {
		case E907_CLK_REG_E907_CLK_SEL_HOSC:
			clock = sunxi_clk_get_hosc_type();
			break;
		case E907_CLK_REG_E907_CLK_SEL_VIDEOPLL2X:
			reg_val = readl(CCU_PLL_VIDEO_CTRL_REG);
			n = (reg_val & PLL_VIDEO_CTRL_REG_PLL_N_CLEAR_MASK) >> PLL_VIDEO_CTRL_REG_PLL_N_OFFSET;
			m = (reg_val & PLL_VIDEO_CTRL_REG_PLL_INPUT_DIV_CLEAR_MASK) >> PLL_VIDEO_CTRL_REG_PLL_INPUT_DIV_OFFSET;
			clock = (sunxi_clk_get_hosc_type() * 2 * (n + 1)) / (m + 1);
			break;
		case E907_CLK_REG_E907_CLK_SEL_RC1M:
		case E907_CLK_REG_E907_CLK_SEL_RC1M0:
			clock = 1;
			break;
		case E907_CLK_REG_E907_CLK_SEL_CPU_PLL:
			pll_cpu_reg = readl(CCU_PLL_CPUX_CTRL_REG);
			n = (pll_cpu_reg & PLL_CPU_CTRL_REG_PLL_N_CLEAR_MASK) >> PLL_CPU_CTRL_REG_PLL_N_OFFSET;
			clock = sunxi_clk_get_hosc_type() * (n + 1);
			break;
		case E907_CLK_REG_E907_CLK_SEL_PERI_PLL_1024M:
			clock = 1024;
			break;
		case E907_CLK_REG_E907_CLK_SEL_PERI_PLL_614M:
		case E907_CLK_REG_E907_CLK_SEL_PERI_PLL_614M0:
			clock = 614;
			break;
		default:
			clock = 1;
	}

	reg_val = readl(SUNXI_CCM_AON_BASE + E907_CLK_REG);
	m = reg_val & E907_CLK_REG_E907_CLK_DIV_CLEAR_MASK;

	printk_debug("CLK: CPU FREQ = %d MHz\n", clock / (m + 1));

	reg_val = read32(CCU_PLL_PERI_CTRL0_REG);
	n = (reg_val & PLL_PERI_CTRL0_REG_PLL_N_CLEAR_MASK) >> PLL_PERI_CTRL0_REG_PLL_N_OFFSET;
	m = reg_val & PLL_PERI_CTRL0_REG_PLL_INPUT_DIV_CLEAR_MASK;
	printk_debug("CLK: PERI FREQ = %lu MHz\r\n", (sunxi_clk_get_hosc_type() * 2 * (n + 1)) / (m + 1));
}

/* we got hosc freq in arch/timer.c */
extern uint32_t current_hosc_freq;

uint32_t sunxi_clk_get_hosc_type() {
	return current_hosc_freq;
}

uint32_t sunxi_clk_get_peri1x_rate() {
	return 192; /* PERI_192M */
}