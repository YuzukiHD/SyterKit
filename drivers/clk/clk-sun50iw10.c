/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file clk-sun50iw10.c
 * @brief Clock driver for the Allwinner A133/R818 (sun50iw10) SoC.
 *
 * Programs the CPU and peripheral PLLs, the AHB/APB dividers, the MBUS clock
 * and the module clocks during early boot, and provides a clock dump helper.
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
#include <drivers/clk/sun50iw10/reg.h>

/**
 * @brief Configure the CPUX PLL and the CPUX/AXI clock dividers.
 *
 * Selects OSC24M as the initial CPUX clock source, programs the PLL_CPUX
 * control register for 1008 MHz, then switches the CPUX clock to PLL_CPUX
 * (CPUX:AXI = 408:136 MHz).
 */
static inline void set_pll_cpux_axi(void)
{
	uint32_t reg_val;
	/* select CPUX  clock src: OSC24M,AXI divide ratio is 2, system apb clk ratio is 4 */
	writel((0 << 24) | (3 << 8) | (1 << 0), SUNXI_CCM_BASE + CCU_CPUX_AXI_CFG_REG);
	udelay(1);

	/* disable pll gating*/
	reg_val = readl(SUNXI_CCM_BASE + CCU_PLL_CPUX_CTRL_REG);
	reg_val &= ~(1 << 27);
	writel(reg_val, SUNXI_CCM_BASE + CCU_PLL_CPUX_CTRL_REG);

	/* set default val: clk is 1008M  ,PLL_OUTPUT= 24M*N/( M*P)*/
	reg_val = readl(SUNXI_CCM_BASE + CCU_PLL_CPUX_CTRL_REG);
	reg_val &= ~((0x3 << 16) | (0xff << 8) | (0x3 << 0));
	reg_val |= (41 << 8);
	writel(reg_val, SUNXI_CCM_BASE + CCU_PLL_CPUX_CTRL_REG);
	/* lock enable */
	reg_val = readl(SUNXI_CCM_BASE + CCU_PLL_CPUX_CTRL_REG);
	reg_val |= (1 << 29);
	writel(reg_val, SUNXI_CCM_BASE + CCU_PLL_CPUX_CTRL_REG);

	/*wait PLL_CPUX stable*/

	while (!(readl(SUNXI_CCM_BASE + CCU_PLL_CPUX_CTRL_REG) & (0x1 << 28)))
		;
	sdelay(20);

	/* enable pll gating*/
	reg_val = readl(SUNXI_CCM_BASE + CCU_PLL_CPUX_CTRL_REG);
	reg_val |= (1 << 27);
	writel(reg_val, SUNXI_CCM_BASE + CCU_PLL_CPUX_CTRL_REG);

	/* lock disable */
	reg_val = readl(SUNXI_CCM_BASE + CCU_PLL_CPUX_CTRL_REG);
	reg_val &= ~(1 << 29);
	writel(reg_val, SUNXI_CCM_BASE + CCU_PLL_CPUX_CTRL_REG);

	/*set and change cpu clk src to PLL_CPUX,  PLL_CPUX:AXI0 = 408M:136M*/
	reg_val = readl(SUNXI_CCM_BASE + CCU_CPUX_AXI_CFG_REG);
	reg_val &= ~(0x03 << 24);
	reg_val |= (0x03 << 24);
	writel(reg_val, SUNXI_CCM_BASE + CCU_CPUX_AXI_CFG_REG);
	udelay(1);
}

/**
 * @brief Enable the PERIPH0 PLL.
 *
 * Programs the default PLL factors, enables lock, then enables the PLL and
 * waits for it to lock. If FEL has already enabled the PLL this is a no-op.
 */
static inline void set_pll_periph0(void)
{
	uint32_t reg_val;

	if ((1U << 31) & readl(SUNXI_CCM_BASE + CCU_PLL_PERI0_CTRL_REG)) {
		/*fel has enable pll_periph0*/
		pr_debug("periph0 has been enabled\n");
		return;
	}

	/* set default val*/
	writel(0x63 << 8, SUNXI_CCM_BASE + CCU_PLL_PERI0_CTRL_REG);

	/* lock enable */
	reg_val = readl(SUNXI_CCM_BASE + CCU_PLL_PERI0_CTRL_REG);
	reg_val |= (1 << 29);
	writel(reg_val, SUNXI_CCM_BASE + CCU_PLL_PERI0_CTRL_REG);

	/* enabe PLL: 600M(1X)  1200M(2x) 2400M(4X) */
	reg_val = readl(SUNXI_CCM_BASE + CCU_PLL_PERI0_CTRL_REG);
	reg_val |= (1 << 31);
	writel(reg_val, SUNXI_CCM_BASE + CCU_PLL_PERI0_CTRL_REG);

	while (!(readl(SUNXI_CCM_BASE + CCU_PLL_PERI0_CTRL_REG) & (0x1 << 28)))
		;
	sdelay(20);

	/* lock disable */
	reg_val = readl(SUNXI_CCM_BASE + CCU_PLL_PERI0_CTRL_REG);
	reg_val &= (~(1 << 29));
	writel(reg_val, SUNXI_CCM_BASE + CCU_PLL_PERI0_CTRL_REG);
}

/**
 * @brief Configure the AHB bus clocks.
 *
 * Sets the AHB1/AHB2 and AHB3 dividers so they run at 200 MHz derived from
 * the PLL6 600 MHz clock.
 */
static inline void set_ahb(void)
{
	/* PLL6:AHB1:APB1 = 600M:200M:100M */
	writel((2 << 0) | (0 << 8), SUNXI_CCM_BASE + CCU_PSI_AHB1_AHB2_CFG_REG);
	writel((0x03 << 24) | readl(SUNXI_CCM_BASE + CCU_PSI_AHB1_AHB2_CFG_REG), SUNXI_CCM_BASE + CCU_PSI_AHB1_AHB2_CFG_REG);
	udelay(1);
	/*PLL6:AHB3 = 600M:200M*/
	writel((2 << 0) | (0 << 8), SUNXI_CCM_BASE + CCU_AHB3_CFG_GREG);
	writel((0x03 << 24) | readl(SUNXI_CCM_BASE + CCU_AHB3_CFG_GREG), SUNXI_CCM_BASE + CCU_AHB3_CFG_GREG);
}

/**
 * @brief Configure the APB1 bus clock.
 *
 * Sets the APB1 divider so it runs at 100 MHz derived from the PLL6 600 MHz
 * clock.
 */
static inline void set_apb(void)
{
	/*PLL6:APB1 = 600M:100M */
	writel((2 << 0) | (1 << 8), SUNXI_CCM_BASE + CCU_APB1_CFG_GREG);
	writel((0x03 << 24) | readl(SUNXI_CCM_BASE + CCU_APB1_CFG_GREG), SUNXI_CCM_BASE + CCU_APB1_CFG_GREG);
	udelay(1);
}

/**
 * @brief Deassert the DMA reset and open the DMA clock gate.
 *
 * Releases the DMA from reset and enables the gating clock so the DMA engine
 * can be used by the boot loader.
 */
static inline void set_pll_dma(void)
{
	/*dma reset*/
	writel(readl(SUNXI_CCM_BASE + CCU_DMA_BGR_REG) | (1 << 16), SUNXI_CCM_BASE + CCU_DMA_BGR_REG);
	udelay(20);
	/*gating clock for dma pass*/
	writel(readl(SUNXI_CCM_BASE + CCU_DMA_BGR_REG) | (1 << 0), SUNXI_CCM_BASE + CCU_DMA_BGR_REG);
}

/**
 * @brief Configure the MBUS clock.
 *
 * Resets the MBUS domain, sets the divider to derive 400 MHz from PLL6(2x),
 * selects that clock source and opens the MBUS clock gate.
 */
static inline void set_pll_mbus(void)
{
	uint32_t reg_val;

	/*reset mbus domain*/
	reg_val = 1 << 30;
	writel(1 << 30, SUNXI_CCM_BASE + CCU_MBUS_CFG_REG);
	udelay(1);

	/* set MBUS div */
	reg_val = readl(SUNXI_CCM_BASE + CCU_MBUS_CFG_REG);
	reg_val |= (2 << 0);
	writel(reg_val, SUNXI_CCM_BASE + CCU_MBUS_CFG_REG);
	udelay(1);

	/* set MBUS clock source to pll6(2x), mbus=pll6/(m+1) = 400M*/
	reg_val = readl(SUNXI_CCM_BASE + CCU_MBUS_CFG_REG);
	reg_val |= (1 << 24);
	writel(reg_val, SUNXI_CCM_BASE + CCU_MBUS_CFG_REG);
	udelay(1);

	/* open MBUS clock */
	reg_val = readl(SUNXI_CCM_BASE + CCU_MBUS_CFG_REG);
	reg_val |= (0X01 << 31);
	writel(reg_val, SUNXI_CCM_BASE + CCU_MBUS_CFG_REG);
	udelay(1);
}

/**
 * @brief Enable the analog calibration circuits.
 *
 * Powers on the analog calibration block used by the ADC and other analog
 * peripherals.
 */
static inline void set_circuits_analog(void)
{
	/* calibration circuits analog enable */
	setbits_le32(SUNXI_RPRCM_BASE + 0x254U, 0x01 << VDD_ADDA_OFF_GATING);
	udelay(1);

	setbits_le32(SUNXI_IOMMU_BASE + 0x160U, 0x01 << CAL_ANA_EN);
	udelay(1);

	clrbits_le32(SUNXI_IOMMU_BASE + 0x160U, 0x01 << CAL_EN);
	udelay(1);

	setbits_le32(SUNXI_IOMMU_BASE + 0x160U, 0x01 << CAL_EN);
	udelay(1);
}

/**
 * @brief Enable the IOMMU gating clock and auto gating.
 *
 * Opens the IOMMU clock gate and enables hardware auto gating so the IOMMU
 * clock is only running when required.
 */
static inline void set_iommu_auto_gating(void)
{
	/*gating clock for iommu*/
	writel(0x01, SUNXI_CCM_BASE + CCU_IOMMU_BGR_REG);
	/*enable auto gating*/
	writel(0x01, SUNXI_SYSCRL_BASE + 0x40U);
}

/**
 * @brief Apply the platform-specific clock configuration.
 *
 * Enables the analog calibration circuits and the IOMMU auto gating.
 */
static inline void set_platform_config(void)
{
	set_circuits_analog();
	set_iommu_auto_gating();
}

/**
 * @brief Enable all module PLLs.
 *
 * Walks the list of module PLL control registers and enables any PLL that is
 * not yet running, waiting for each one to lock.
 */
static inline void set_modules_clock(void)
{
	uint32_t reg_val, i;
	uint32_t ccmu_pll_addr[] = {
		SUNXI_CCM_BASE + CCU_PLL_PERI0_CTRL_REG,  SUNXI_CCM_BASE + CCU_PLL_PERI1_CTRL_REG,  SUNXI_CCM_BASE + CCU_PLL_GPU_CTRL_REG,
		SUNXI_CCM_BASE + CCU_PLL_VIDE00_CTRL_REG, SUNXI_CCM_BASE + CCU_PLL_VIDE01_CTRL_REG, SUNXI_CCM_BASE + CCU_PLL_VIDE02_CTRL_REG,
		SUNXI_CCM_BASE + CCU_PLL_VIDE03_CTRL_REG, SUNXI_CCM_BASE + CCU_PLL_VE_CTRL_REG,	    SUNXI_CCM_BASE + CCU_PLL_COM_CTRL_REG,
		SUNXI_CCM_BASE + CCU_PLL_AUDIO_CTRL_REG,
	};

	for (i = 0; i < sizeof(ccmu_pll_addr) / sizeof(ccmu_pll_addr[0]); i++) {
		reg_val = readl(ccmu_pll_addr[i]);
		if (!(reg_val & (1 << 31))) {
			writel(reg_val, ccmu_pll_addr[i]);

			reg_val = readl(ccmu_pll_addr[i]);
			writel(reg_val | (1 << 31), ccmu_pll_addr[i]);
			/* lock enable */
			reg_val = readl(ccmu_pll_addr[i]);
			reg_val |= (1 << 29);
			writel(reg_val, ccmu_pll_addr[i]);

			while (!(readl(ccmu_pll_addr[i]) & (0x1 << 28)))
				;
			udelay(20);

			reg_val = readl(ccmu_pll_addr[i]);
			reg_val &= ~(1 << 29);
			writel(reg_val, ccmu_pll_addr[i]);
		}
	}
}

/**
 * @brief Initialize the SoC clocks.
 *
 * Applies the platform configuration, then programs the CPUX PLL, PERIPH0
 * PLL, AHB/APB dividers, DMA, MBUS and module clocks.
 */
void sunxi_clk_init(void)
{
	pr_debug("Set SoC 1855 (A133/R818) CLK Start.\n");
	set_platform_config();
	set_pll_cpux_axi();
	set_pll_periph0();
	set_ahb();
	set_apb();
	set_pll_dma();
	set_pll_mbus();
	set_modules_clock();
	pr_debug("set pll end\n");
	return;
}

/**
 * @brief Dump the current SoC clock configuration.
 *
 * Prints the CPUX clock source and the computed frequencies of the CPU,
 * PERIPH0/PERIPH1, DDR0/DDR1 and HSIC PLLs.
 */
void sunxi_clk_dump(void)
{
	uint32_t reg32;
	uint32_t cpu_clk_src;
	uint32_t plln, pllm;
	uint8_t p0;
	uint8_t p1;
	const char *clock_str;

	/* PLL CPU */
	reg32 = read32(SUNXI_CCM_BASE + CCU_CPUX_AXI_CFG_REG);
	cpu_clk_src = (reg32 >> 24) & 0x7;

	switch (cpu_clk_src) {
	case 0x0:
		clock_str = "OSC24M";
		break;

	case 0x1:
		clock_str = "CLK32";
		break;

	case 0x2:
		clock_str = "CLK16M_RC";
		break;

	case 0x3:
		clock_str = "PLL_CPU";
		break;

	case 0x4:
		clock_str = "PLL_PERI0(1X)";
		break;

	default:
		clock_str = "reserved";
	}

	reg32 = read32(SUNXI_CCM_BASE + CCU_PLL_CPUX_CTRL_REG);
	p0 = (reg32 >> 16) & 0x03;
	if (p0 == 0) {
		p1 = 1;
	} else if (p0 == 1) {
		p1 = 2;
	} else if (p0 == 2) {
		p1 = 4;
	} else {
		p1 = 1;
	}

	pr_debug("CLK: CPU PLL=%s FREQ=%luMHz\r\n", clock_str, ((((reg32 >> 8) & 0xff) + 1) * 24 / p1));

	/* PLL PERIx */
	reg32 = read32(SUNXI_CCM_BASE + CCU_PLL_PERI0_CTRL_REG);
	if (reg32 & (1 << 31)) {
		plln = ((reg32 >> 8) & 0xff) + 1;
		pllm = (reg32 & 0x01) + 1;
		p0 = ((reg32 >> 16) & 0x03) + 1;
		p1 = ((reg32 >> 20) & 0x03) + 1;

		pr_debug("CLK: PLL_PERI0 (2X)=%luMHz, (1X)=%luMHz, (1200M)=%luMHz\r\n", (24 * plln) / (pllm * p0), (24 * plln) / (pllm * p0) >> 1, (24 * plln) / (pllm * p1));
	} else {
		pr_debug("CLK: PLL_PERI0 disabled\r\n");
	}

	/* PLL PERIx */
	reg32 = read32(SUNXI_CCM_BASE + CCU_PLL_PERI1_CTRL_REG);
	if (reg32 & (1 << 31)) {
		plln = ((reg32 >> 8) & 0xff) + 1;
		pllm = (reg32 & 0x01) + 1;
		p0 = ((reg32 >> 16) & 0x03) + 1;
		p1 = ((reg32 >> 20) & 0x03) + 1;

		pr_debug("CLK: PLL_PERI1 (2X)=%luMHz, (1X)=%luMHz, (1200M)=%luMHz\r\n", (24 * plln) / (pllm * p0), (24 * plln) / (pllm * p0) >> 1, (24 * plln) / (pllm * p1));
	} else {
		pr_debug("CLK: PLL_PERI1 disabled\r\n");
	}

	/* PLL DDR0 */
	reg32 = read32(SUNXI_CCM_BASE + CCU_PLL_DDR0_CTRL_REG);
	if (reg32 & (1 << 31)) {
		plln = ((reg32 >> 8) & 0xff) + 1;

		pllm = (reg32 & 0x01) + 1;
		p1 = ((reg32 >> 1) & 0x1) + 1;
		p0 = (reg32 & 0x01) + 1;

		pr_debug("CLK: PLL_DDR0=%luMHz\r\n", (24 * plln) / (p0 * p1));

	} else {
		pr_debug("CLK: PLL_DDR0 disabled\r\n");
	}

	/* PLL DDR1 */
	reg32 = read32(SUNXI_CCM_BASE + CCU_PLL_DDR1_CTRL_REG);
	if (reg32 & (1 << 31)) {
		plln = ((reg32 >> 8) & 0xff) + 1;

		pllm = (reg32 & 0x01) + 1;
		p1 = ((reg32 >> 1) & 0x1) + 1;
		p0 = (reg32 & 0x01) + 1;

		pr_debug("CLK: PLL_DDR1=%luMHz\r\n", (24 * plln) / (p0 * p1));

	} else {
		pr_debug("CLK: PLL_DDR1 disabled\r\n");
	}

	/* PLL HSIC */
	reg32 = read32(SUNXI_CCM_BASE + CCU_PLL_HSIC_CTRL_REG);
	if (reg32 & (1 << 31)) {
		plln = ((reg32 >> 8) & 0xff) + 1;

		pllm = (reg32 & 0x01) + 1;
		p1 = ((reg32 >> 1) & 0x1) + 1;
		p0 = (reg32 & 0x01) + 1;

		pr_debug("CLK: HSIC=%luMHz\r\n", (24 * plln) / (p0 * p1));

	} else {
		pr_debug("CLK: HSIC disabled\r\n");
	}
}
