/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file clk-sun300iw1.c
 * @brief Clock driver for the Allwinner sun300iw1 SoC.
 *
 * Programs the CPU, PERI, CSI and VIDEO PLLs together with the AHB/APB
 * dividers during early boot, and provides a clock dump helper.
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
#include <drivers/clk/sun300iw1/clk.h>
#include <drivers/clk/sun300iw1/reg.h>

/**
 * @brief General purpose PLL configuration helper.
 *
 * Programs the D and N divider factors, then enables or disables the PLL,
 * LDO, lock action and output gate as requested, waiting for the PLL to lock
 * when it is enabled.
 *
 * @param[in] pll_addr        PLL control register address.
 * @param[in] en              1 to enable the PLL, 0 to disable it.
 * @param[in] output_gate_en  1 to enable the output gate, 0 to disable it.
 * @param[in] pll_d           D divider value.
 * @param[in] pll_d_off       Bit offset of the D divider field.
 * @param[in] pll_n           N divider factor value.
 */
static void set_pll_general(uintptr_t pll_addr, uint32_t en, uint32_t output_gate_en, uint32_t pll_d, uint32_t pll_d_off, uint32_t pll_n)
{
	uint32_t pll_en;
	uint32_t pll_ldo_en;
	uint32_t pll_lock_en;
	uint32_t pll_output_gate;

	if (en == 1) {
		pll_en = PLL_Enable;
		pll_ldo_en = PLL_LDO_Enable;
		pll_lock_en = PLL_LOCK_EN_Enable;
	} else {
		pll_en = PLL_Disable;
		pll_ldo_en = PLL_LDO_Disable;
		pll_lock_en = PLL_LOCK_EN_Disable;
	}

	pll_output_gate = output_gate_en ? PLL_OUTPUT_GATE_Enable : PLL_OUTPUT_GATE_Disable;
	clrsetbits_le32(pll_addr, PLL_D_MASK, pll_d << pll_d_off);
	clrsetbits_le32(pll_addr, PLL_N_MASK, pll_n << PLL_N_OFFSET);
	clrsetbits_le32(pll_addr, PLL_LDO_MASK | PLL_OUTPUT_GATE_MASK | PLL_EN_MASK, pll_en | pll_ldo_en | PLL_OUTPUT_GATE_Disable);
	clrsetbits_le32(pll_addr, PLL_LOCK_EN_MASK, pll_lock_en);

	if (en) {
		while (!(readl(pll_addr) & PLL_LOCK_MASK))
			;
	}

	clrsetbits_le32(pll_addr, PLL_OUTPUT_GATE_MASK, pll_output_gate);

	return;
}

/**
 * @brief Configure the E907 RISC-V core clock.
 *
 * Sets the E907 divider to 1 and selects the PERI PLL 614 MHz clock as the
 * core clock source.
 */
static void set_pll_e90x(void)
{
	clrsetbits_le32(SUNXI_CCU_AON_BASE + E907_CLK_REG, E907_CLK_REG_E907_CLK_DIV_CLEAR_MASK, CCU_E90X_CLK_CPU_M_1 << E907_CLK_REG_E907_CLK_DIV_OFFSET);
	clrsetbits_le32(SUNXI_CCU_AON_BASE + E907_CLK_REG, E907_CLK_REG_E907_CLK_SEL_CLEAR_MASK, E907_CLK_REG_E907_CLK_SEL_PERI_PLL_614M << E907_CLK_REG_E907_CLK_SEL_OFFSET);
	return;
}

/**
 * @brief Configure the A27L2 core clock.
 *
 * Sets the A27L2 divider to 1, selects the CPU PLL as the clock source and
 * enables the A27L2 clock.
 */
static void set_pll_a27l2(void)
{
	clrsetbits_le32(SUNXI_CCU_AON_BASE + A27L2_CLK_REG, A27L2_CLK_REG_A27L2_CLK_DIV_CLEAR_MASK, CCU_A27_CLK_CPU_M_1 << A27L2_CLK_REG_A27L2_CLK_DIV_OFFSET);
	clrsetbits_le32(SUNXI_CCU_AON_BASE + A27L2_CLK_REG, A27L2_CLK_REG_A27L2_CLK_SEL_CLEAR_MASK, A27L2_CLK_REG_A27L2_CLK_SEL_CPU_PLL << A27L2_CLK_REG_A27L2_CLK_SEL_OFFSET);
	clrsetbits_le32(SUNXI_CCU_AON_BASE + A27L2_CLK_REG, A27L2_CLK_REG_A27L2_CLK_EN_CLEAR_MASK, A27L2_CLK_REG_A27L2_CLK_EN_CLOCK_IS_ON << A27L2_CLK_REG_A27L2_CLK_EN_OFFSET);
	return;
}

/**
 * @brief Configure the PERI PLL control0 register.
 *
 * Programs the N factor and input divider, enables or disables the PLL, LDO,
 * lock action and output gate as requested, and waits for the PLL to lock.
 *
 * @param[in] en              1 to enable the PLL, 0 to disable it.
 * @param[in] output_gate_en  1 to enable the output gate, 0 to disable it.
 * @param[in] pll_n           N divider factor value.
 * @param[in] pll_m           Input divider value.
 */
static void set_pll_peri_ctrl0(uint32_t en, uint32_t output_gate_en, uint32_t pll_n, uint32_t pll_m)
{
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

	clrsetbits_le32(SUNXI_CCU_AON_BASE + PLL_PERI_CTRL0_REG, PLL_PERI_CTRL0_REG_PLL_INPUT_DIV_CLEAR_MASK, pll_m);
	clrsetbits_le32(SUNXI_CCU_AON_BASE + PLL_PERI_CTRL0_REG, PLL_PERI_CTRL0_REG_PLL_N_CLEAR_MASK, pll_n);

	clrsetbits_le32(SUNXI_CCU_AON_BASE + PLL_PERI_CTRL0_REG,
			PLL_PERI_CTRL0_REG_PLL_EN_CLEAR_MASK | PLL_PERI_CTRL0_REG_PLL_LDO_EN_CLEAR_MASK | PLL_PERI_CTRL0_REG_PLL_OUTPUT_GATE_CLEAR_MASK,
			pll_en | pll_ldo_en | pll_output_gate);

	clrsetbits_le32(SUNXI_CCU_AON_BASE + PLL_PERI_CTRL0_REG, PLL_PERI_CTRL0_REG_LOCK_ENABLE_CLEAR_MASK, pll_lock_en);

	while (!(readl(SUNXI_CCU_AON_BASE + PLL_PERI_CTRL0_REG) & PLL_PERI_CTRL0_REG_LOCK_CLEAR_MASK))
		;

	if (output_gate_en == 1) {
		pll_output_gate = PLL_PERI_CTRL0_REG_PLL_OUTPUT_GATE_ENABLE << PLL_PERI_CTRL0_REG_PLL_OUTPUT_GATE_OFFSET;
		clrsetbits_le32(SUNXI_CCU_AON_BASE + PLL_PERI_CTRL0_REG, PLL_PERI_CTRL0_REG_PLL_OUTPUT_GATE_CLEAR_MASK, pll_output_gate);
	}
}

/**
 * @brief Configure the PERI PLL control1 register.
 *
 * Programs the PERI PLL control1 register to enable the peripheral output
 * dividers.
 */
static void set_pll_peri_ctrl1(void)
{
	uint32_t reg;
	reg = readl(SUNXI_CCU_AON_BASE + PLL_PERI_CTRL1_REG);
	// reg |= PLL_PERI_CTRL1_REG_PLL_PERI_CKO_192_EN_ENABLE
	//        << PLL_PERI_CTRL1_REG_PLL_PERI_CKO_192_EN_OFFSET;
	reg = 0xFFFF;
	writel(reg, SUNXI_CCU_AON_BASE + PLL_PERI_CTRL1_REG);
	return;
}

/* pll peri hosc*2N/M = 3072M  hardware * 2 */
/**
 * @brief Configure the PERI PLL.
 *
 * Enables the PERI PLL at 3072 MHz when it is not already running, choosing
 * the N/M factors according to the HOSC rate, and configures the control1
 * register.
 */
static void set_pll_peri(void)
{
	if (!(readl(SUNXI_CCU_AON_BASE + PLL_PERI_CTRL0_REG) & PLL_PERI_CTRL0_REG_PLL_EN_CLEAR_MASK)) {
		if (sun300iw1_clk_get_hosc_rate() == HOSC_FREQ_40M) {
			set_pll_peri_ctrl0(PLL_PERI_CTRL0_REG_PLL_EN_ENABLE, PLL_PERI_CTRL0_REG_PLL_OUTPUT_GATE_ENABLE, CCU_AON_PLL_CPU_N_192, CCU_AON_PLL_CPU_M_5);
		} else {
			set_pll_peri_ctrl0(PLL_PERI_CTRL0_REG_PLL_EN_ENABLE, PLL_PERI_CTRL0_REG_PLL_OUTPUT_GATE_ENABLE, CCU_AON_PLL_CPU_N_192, CCU_AON_PLL_CPU_M_3);
		}
	}
	set_pll_peri_ctrl1();
	return;
}

/* pll csi rate = 675M = hosc / 4 * N , N = N(INT) + N(FRAC) */
/**
 * @brief Configure the CSI PLL.
 *
 * Programs the N factor and input divider for 675 MHz, enables the PLL, LDO
 * and SDM, applies the sigma-delta pattern settings and waits for the PLL to
 * lock before enabling the output.
 */
static void set_pll_csi(void)
{
	uint32_t n, input_div, wave_bot;

	/* Set N、M */
	if (sun300iw1_clk_get_hosc_rate() == HOSC_FREQ_40M) {
		n = CCU_AON_PLL_CPU_N_67;
		wave_bot = 0xc0010000;
		input_div = PLL_CSI_CTRL_REG_PLL_INPUT_DIV_4;
	} else {
		n = CCU_AON_PLL_CPU_N_56;
		wave_bot = 0xc0008000;
		input_div = PLL_CSI_CTRL_REG_PLL_INPUT_DIV_2;
	}
	clrsetbits_le32(SUNXI_CCU_AON_BASE + PLL_CSI_CTRL_REG, PLL_CSI_CTRL_REG_PLL_FACTOR_N_CLEAR_MASK, n << PLL_CSI_CTRL_REG_PLL_FACTOR_N_OFFSET);
	clrsetbits_le32(SUNXI_CCU_AON_BASE + PLL_CSI_CTRL_REG, PLL_CSI_CTRL_REG_PLL_INPUT_DIV_CLEAR_MASK, input_div << PLL_CSI_CTRL_REG_PLL_INPUT_DIV_OFFSET);

	/* Enable PLL_EN PLL_LEO_EN */
	clrsetbits_le32(SUNXI_CCU_AON_BASE + PLL_CSI_CTRL_REG, PLL_CSI_CTRL_REG_PLL_EN_CLEAR_MASK, PLL_CSI_CTRL_REG_PLL_EN_ENABLE << PLL_CSI_CTRL_REG_PLL_EN_OFFSET);
	clrsetbits_le32(SUNXI_CCU_AON_BASE + PLL_PERI_CTRL0_REG, PLL_CSI_CTRL_REG_PLL_LDO_EN_CLEAR_MASK, PLL_CSI_CTRL_REG_PLL_LDO_EN_ENABLE << PLL_CSI_CTRL_REG_PLL_LDO_EN_OFFSET);

	/* Enable SDM */
	clrsetbits_le32(SUNXI_CCU_AON_BASE + PLL_CSI_CTRL_REG, PLL_CSI_CTRL_REG_PLL_SDM_EN_CLEAR_MASK, PLL_CSI_CTRL_REG_PLL_SDM_EN_ENABLE << PLL_CSI_CTRL_REG_PLL_SDM_EN_OFFSET);

	/* Set WAVE_BOT and SPR_FREQ_MODE of PAT0 */
	clrsetbits_le32(SUNXI_CCU_AON_BASE + PLL_CSI_PAT0_CTRL_REG, PLL_CSI_PAT0_CTRL_REG_WAVE_BOT_CLEAR_MASK, wave_bot << PLL_CSI_PAT0_CTRL_REG_WAVE_BOT_OFFSET);

	/* Enable SIG_DELT_PAT_EN of PAT1 */
	clrsetbits_le32(SUNXI_CCU_AON_BASE + PLL_CSI_PAT1_CTRL_REG, PLL_CSI_PAT1_CTRL_REG_SIG_DELT_PAT_EN_CLEAR_MASK, 0x1 << PLL_CSI_PAT1_CTRL_REG_SIG_DELT_PAT_EN_OFFSET);

	/* Disable OUTPUT and Enable LOCK_ENABLE */
	clrsetbits_le32(SUNXI_CCU_AON_BASE + PLL_CSI_CTRL_REG, PLL_CSI_CTRL_REG_PLL_OUTPUT_GATE_CLEAR_MASK,
			PLL_CSI_CTRL_REG_PLL_OUTPUT_GATE_DISABLE << PLL_CSI_CTRL_REG_PLL_OUTPUT_GATE_OFFSET);
	clrsetbits_le32(SUNXI_CCU_AON_BASE + PLL_CSI_CTRL_REG, PLL_CSI_CTRL_REG_LOCK_ENABLE_CLEAR_MASK, PLL_CSI_CTRL_REG_LOCK_ENABLE_ENABLE << PLL_CSI_CTRL_REG_LOCK_ENABLE_OFFSET);

	/* Wait Lock_status */
	while (!(readl(SUNXI_CCU_AON_BASE + PLL_CSI_CTRL_REG) & PLL_CSI_CTRL_REG_LOCK_CLEAR_MASK))
		;

	/* Enable Output */
	clrsetbits_le32(SUNXI_CCU_AON_BASE + PLL_CSI_CTRL_REG, PLL_CSI_CTRL_REG_PLL_OUTPUT_GATE_CLEAR_MASK,
			PLL_CSI_CTRL_REG_PLL_OUTPUT_GATE_ENABLE << PLL_CSI_CTRL_REG_PLL_OUTPUT_GATE_OFFSET);
}

/* pll video = 1200M = hosc * N */
/**
 * @brief Configure the VIDEO PLL.
 *
 * Programs the N factor and input divider for 1200 MHz, enables the lock
 * action, waits for the PLL to lock and enables the output gate.
 */
static void set_pll_video(void)
{
	/* Close Lock_en and Output_gate */
	clrsetbits_le32(SUNXI_CCU_AON_BASE + PLL_VIDEO_CTRL_REG, PLL_VIDEO_CTRL_REG_LOCK_ENABLE_CLEAR_MASK,
			PLL_VIDEO_CTRL_REG_LOCK_ENABLE_DISABLE << PLL_VIDEO_CTRL_REG_LOCK_ENABLE_OFFSET);
	clrsetbits_le32(SUNXI_CCU_AON_BASE + PLL_VIDEO_CTRL_REG, PLL_VIDEO_CTRL_REG_PLL_OUTPUT_GATE_CLEAR_MASK,
			PLL_VIDEO_CTRL_REG_PLL_OUTPUT_GATE_DISABLE << PLL_VIDEO_CTRL_REG_PLL_OUTPUT_GATE_OFFSET);

	/* Set N、M */
	if (sun300iw1_clk_get_hosc_rate() == HOSC_FREQ_40M) {
		clrsetbits_le32(SUNXI_CCU_AON_BASE + PLL_VIDEO_CTRL_REG, PLL_VIDEO_CTRL_REG_PLL_N_CLEAR_MASK, CCU_AON_PLL_CPU_N_30 << PLL_VIDEO_CTRL_REG_PLL_N_OFFSET);
	} else {
		clrsetbits_le32(SUNXI_CCU_AON_BASE + PLL_VIDEO_CTRL_REG, PLL_VIDEO_CTRL_REG_PLL_N_CLEAR_MASK, CCU_AON_PLL_CPU_N_50 << PLL_VIDEO_CTRL_REG_PLL_N_OFFSET);
	}
	clrsetbits_le32(SUNXI_CCU_AON_BASE + PLL_VIDEO_CTRL_REG, PLL_VIDEO_CTRL_REG_PLL_INPUT_DIV_CLEAR_MASK,
			PLL_VIDEO_CTRL_REG_PLL_INPUT_DIV_1 << PLL_VIDEO_CTRL_REG_PLL_INPUT_DIV_OFFSET);

	/* Enable Lock_en */
	clrsetbits_le32(SUNXI_CCU_AON_BASE + PLL_VIDEO_CTRL_REG, PLL_VIDEO_CTRL_REG_LOCK_ENABLE_CLEAR_MASK,
			PLL_VIDEO_CTRL_REG_LOCK_ENABLE_ENABLE << PLL_VIDEO_CTRL_REG_LOCK_ENABLE_OFFSET);

	/* Wait Lock_status */
	while (!(readl(SUNXI_CCU_AON_BASE + PLL_VIDEO_CTRL_REG) & PLL_VIDEO_CTRL_REG_LOCK_CLEAR_MASK))
		;

	/* Enable Output */
	clrsetbits_le32(SUNXI_CCU_AON_BASE + PLL_VIDEO_CTRL_REG, PLL_VIDEO_CTRL_REG_PLL_OUTPUT_GATE_CLEAR_MASK,
			PLL_VIDEO_CTRL_REG_PLL_OUTPUT_GATE_ENABLE << PLL_VIDEO_CTRL_REG_PLL_OUTPUT_GATE_OFFSET);
}

// ahb = 768/4 = 192M
/**
 * @brief Configure the AHB bus clock.
 *
 * Sets the AHB divider to derive 192 MHz and selects the PERI 768 MHz clock
 * as the source.
 */
static void set_ahb(void)
{
	clrsetbits_le32(SUNXI_CCU_AON_BASE + AHB_CLK_REG, AHB_CLK_REG_AHB_CLK_DIV_CLEAR_MASK, CCU_AON_PLL_CPU_M_4 << AHB_CLK_REG_AHB_CLK_DIV_OFFSET);
	udelay(2);
	clrsetbits_le32(SUNXI_CCU_AON_BASE + AHB_CLK_REG, AHB_CLK_REG_AHB_SEL_CLEAR_MASK, AHB_CLK_REG_AHB_SEL_PERI_768M << AHB_CLK_REG_AHB_SEL_OFFSET);
	udelay(2);
	return;
}

// apb = 384/4 = 96M
/**
 * @brief Configure the APB bus clock.
 *
 * Sets the APB divider to derive 96 MHz and selects the PERI 384 MHz clock as
 * the source.
 */
static void set_apb(void)
{
	clrsetbits_le32(SUNXI_CCU_AON_BASE + APB_CLK_REG, APB_CLK_REG_APB_CLK_DIV_CLEAR_MASK, CCU_AON_PLL_CPU_M_4 << APB_CLK_REG_APB_CLK_DIV_OFFSET);
	udelay(2);
	clrsetbits_le32(SUNXI_CCU_AON_BASE + APB_CLK_REG, APB_CLK_REG_APB_SEL_CLEAR_MASK, APB_CLK_REG_APB_SEL_PERI_384M << APB_CLK_REG_APB_SEL_OFFSET);
	udelay(2);
	return;
}

// 192M
/**
 * @brief Configure the APB_SPEC bus clock.
 *
 * Selects the PERI 192 MHz clock as the APB_SPEC clock source.
 */
static void set_apb_spec(void)
{
	clrsetbits_le32(SUNXI_CCU_AON_BASE + APB_SPEC_CLK_REG, APB_SPEC_CLK_REG_APB_SPEC_SEL_CLEAR_MASK | APB_SPEC_CLK_REG_APB_SPEC_CLK_DIV_CLEAR_MASK,
			APB_SPEC_CLK_REG_APB_SPEC_SEL_PERI_192M << APB_SPEC_CLK_REG_APB_SPEC_SEL_OFFSET);
	return;
}

/**
 * @brief Initialize the SoC clocks.
 *
 * Programs the CPU and VIDEO PLLs according to the detected HOSC rate, then
 * configures the E907/A27L2 core clocks and the AHB/APB, VIDEO and CSI
 * clocks.
 */
void sunxi_clk_init(void)
{
	/* detect hosc */
	if (sun300iw1_clk_get_hosc_rate() == HOSC_FREQ_40M) {
		set_pll_general(SUNXI_CCU_AON_BASE + PLL_CPU_CTRL_REG, PLL_CPU_CTRL_REG_PLL_EN_ENABLE, PLL_CPU_CTRL_REG_PLL_OUTPUT_GATE_ENABLE, CCU_AON_PLL_CPU_D_1, 2,
				CCU_AON_PLL_CPU_N_27);
		if (!(readl(SUNXI_CCU_AON_BASE + PLL_VIDEO_CTRL_REG) & PLL_CPU_CTRL_REG_PLL_EN_CLEAR_MASK)) {
			set_pll_general(SUNXI_CCU_AON_BASE + PLL_VIDEO_CTRL_REG, PLL_CPU_CTRL_REG_PLL_EN_ENABLE, PLL_CPU_CTRL_REG_PLL_OUTPUT_GATE_ENABLE, CCU_AON_PLL_CPU_D_4, 1,
					CCU_AON_PLL_CPU_N_118);
		}
	} else {
		set_pll_general(SUNXI_CCU_AON_BASE + PLL_CPU_CTRL_REG, PLL_CPU_CTRL_REG_PLL_EN_ENABLE, PLL_CPU_CTRL_REG_PLL_OUTPUT_GATE_ENABLE, CCU_AON_PLL_CPU_D_1, 2,
				CCU_AON_PLL_CPU_N_45);
		if (!(readl(SUNXI_CCU_AON_BASE + PLL_VIDEO_CTRL_REG) & PLL_CPU_CTRL_REG_PLL_EN_CLEAR_MASK)) {
			set_pll_general(SUNXI_CCU_AON_BASE + PLL_VIDEO_CTRL_REG, PLL_CPU_CTRL_REG_PLL_EN_ENABLE, PLL_CPU_CTRL_REG_PLL_OUTPUT_GATE_ENABLE, CCU_AON_PLL_CPU_D_2, 1,
					CCU_AON_PLL_CPU_N_99);
		}
	}

	set_pll_e90x();
	set_pll_a27l2();
	set_ahb();
	set_apb();
	set_pll_video();
	set_pll_csi();
}

/**
 * @brief Pre-initialize the SoC clocks.
 *
 * Configures the APB_SPEC clock and the PERI PLL before the main clock init
 * runs.
 */
void sunxi_clk_preinit(void)
{
	set_apb_spec();
	set_pll_peri();
}

/**
 * @brief Dump the current SoC clock configuration.
 *
 * Prints the HOSC type, the E907 CPU frequency and the PERI PLL frequency.
 */
void sunxi_clk_dump(void)
{
	uint32_t reg_val = 0;
	uint32_t n, m, clock_src, clock, pll_cpu_reg;

	pr_debug("SoC HOSC Type = %d MHz\n", sun300iw1_clk_get_hosc_rate());

	reg_val = readl(SUNXI_CCU_AON_BASE + E907_CLK_REG);
	clock_src = ((reg_val & E907_CLK_REG_E907_CLK_SEL_CLEAR_MASK) >> E907_CLK_REG_E907_CLK_SEL_OFFSET);
	switch (clock_src) {
	case E907_CLK_REG_E907_CLK_SEL_HOSC:
		clock = sun300iw1_clk_get_hosc_rate();
		break;
	case E907_CLK_REG_E907_CLK_SEL_VIDEOPLL2X:
		reg_val = readl(SUNXI_CCU_AON_BASE + PLL_VIDEO_CTRL_REG);
		n = (reg_val & PLL_VIDEO_CTRL_REG_PLL_N_CLEAR_MASK) >> PLL_VIDEO_CTRL_REG_PLL_N_OFFSET;
		m = (reg_val & PLL_VIDEO_CTRL_REG_PLL_INPUT_DIV_CLEAR_MASK) >> PLL_VIDEO_CTRL_REG_PLL_INPUT_DIV_OFFSET;
		clock = (sun300iw1_clk_get_hosc_rate() * 2 * (n + 1)) / (m + 1);
		break;
	case E907_CLK_REG_E907_CLK_SEL_RC1M:
	case E907_CLK_REG_E907_CLK_SEL_RC1M0:
		clock = 1;
		break;
	case E907_CLK_REG_E907_CLK_SEL_CPU_PLL:
		pll_cpu_reg = readl(SUNXI_CCU_AON_BASE + PLL_CPU_CTRL_REG);
		n = (pll_cpu_reg & PLL_CPU_CTRL_REG_PLL_N_CLEAR_MASK) >> PLL_CPU_CTRL_REG_PLL_N_OFFSET;
		clock = sun300iw1_clk_get_hosc_rate() * (n + 1);
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

	reg_val = readl(SUNXI_CCU_AON_BASE + E907_CLK_REG);
	m = reg_val & E907_CLK_REG_E907_CLK_DIV_CLEAR_MASK;

	pr_debug("CLK: CPU FREQ = %d MHz\n", clock / (m + 1));

	reg_val = read32(SUNXI_CCU_AON_BASE + PLL_PERI_CTRL0_REG);
	n = (reg_val & PLL_PERI_CTRL0_REG_PLL_N_CLEAR_MASK) >> PLL_PERI_CTRL0_REG_PLL_N_OFFSET;
	m = reg_val & PLL_PERI_CTRL0_REG_PLL_INPUT_DIV_CLEAR_MASK;
	pr_debug("CLK: PERI FREQ = %lu MHz\r\n", (sun300iw1_clk_get_hosc_rate() * 2 * (n + 1)) / (m + 1));
}

/* we got hosc freq in arch/timer.c */
extern uint32_t current_hosc_freq;

/**
 * @brief Get the high-speed oscillator (HOSC) frequency.
 *
 * @return HOSC frequency in MHz as recorded by the timer code.
 */
uint32_t sun300iw1_clk_get_hosc_rate(void)
{
	return current_hosc_freq;
}
