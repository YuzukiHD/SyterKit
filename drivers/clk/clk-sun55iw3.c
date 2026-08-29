/* SPDX-License-Identifier: GPL-2.0+ */
#define pr_fmt(fmt) "clk-sun55iw3: " fmt

/**
 * @file clk-sun55iw3.c
 * @brief Clock driver for the Allwinner A523/A527/MR527/T527 (sun55iw3) SoC.
 *
 * Programs the CPU, PERIPH and module PLLs together with the AHB/APB and MBUS
 * clocks during early boot, and provides a clock dump helper.
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
#include <drivers/clk/sun55iw3/clk.h>
#include <drivers/clk/sun55iw3/reg.h>

/**
 * @struct core_pll_freq_fact
 * @brief PLL divider factors used to program a CPU core PLL.
 */
typedef struct {
	int FactorN;  /**< N divider factor. */
	int FactorM0; /**< M0 divider factor. */
	int FactorM1; /**< M1 divider factor. */
	int FactorP;  /**< P divider factor. */
} core_pll_freq_fact;

/**
 * @brief Set a single bit in a memory mapped register.
 *
 * @param[in] cpux Register address.
 * @param[in] bit  Bit number to set.
 */
static void set_bit(uintptr_t cpux, u8 bit)
{
	uint32_t reg_val;
	reg_val = readl(cpux);
	reg_val |= (1 << bit);
	writel(reg_val, cpux);
	pr_trace("set_bit cpux = 0x%08x, bit = %d\n", cpux, bit);
}

/**
 * @brief Clear a single bit in a memory mapped register.
 *
 * @param[in] cpux Register address.
 * @param[in] bit  Bit number to clear.
 */
static void clear_bit(uintptr_t cpux, u8 bit)
{
	uint32_t reg_val;
	reg_val = readl(cpux);
	reg_val &= ~(1 << bit);
	writel(reg_val, cpux);
	pr_trace("clear_bit cpux = 0x%08x, bit = %d\n", cpux, bit);
}

/**
 * @brief Enable a CPU PLL with the given divider factors.
 *
 * Writes the default register value, programs the PLL factors from @p CPUx,
 * then enables the PLL, LDO, lock and update bits and waits for the PLL to
 * lock.
 *
 * @param[in] cpux        PLL control register address.
 * @param[in] CPUx        PLL divider factors.
 * @param[in] default_val Default value written to the control register.
 */
static void enable_pll(uintptr_t cpux, core_pll_freq_fact *CPUx, uint32_t default_val)
{
	uint32_t reg_val;

	writel(default_val, cpux);
	/* disable pll gating*/
	clear_bit(cpux, PLL_CPU1_CTRL_REG_PLL_OUTPUT_GATE_OFFSET);

	/*set  PLL_CPUx ,PLL_OUTPUT= 24M*N/P/(M0*M1) */
	reg_val = readl(cpux);
	reg_val &= ~((0x3 << 20) | (0xf << 16) | (0xff << 8) | (0xf << 0));
	reg_val |= ((CPUx->FactorM0 << 20) | (CPUx->FactorP << 16) | (CPUx->FactorN << 8) | (CPUx->FactorM1 << 0));
	writel(reg_val, cpux);

	/* pll  enable */
	set_bit(cpux, PLL_CPU1_CTRL_REG_PLL_EN_OFFSET);

	/* pll ldo enable */
	set_bit(cpux, PLL_CPU1_CTRL_REG_PLL_LDO_EN_OFFSET);
	sdelay(5);

	/* lock enable */
	set_bit(cpux, PLL_CPU1_CTRL_REG_LOCK_ENABLE_OFFSET);

	/* enable update bit */
	set_bit(cpux, 26);

	/*wait PLL_CPUX stable*/
	while (!(readl(cpux) & (0x1 << PLL_CPU1_CTRL_REG_LOCK_OFFSET)))
		;
	sdelay(20);

	/* enable pll gating*/
	set_bit(cpux, PLL_CPU1_CTRL_REG_PLL_OUTPUT_GATE_OFFSET);

	clear_bit(cpux, PLL_CPU1_CTRL_REG_LOCK_ENABLE_OFFSET);

	/* enable update bit */
	set_bit(cpux, 26);
}

/**
 * @brief Configure the CPU and DSU PLLs and select the core clock sources.
 *
 * Enables PLL_CPU1 (core0~3), PLL_CPU3 (core4~7) and PLL_CPU2 (DSU), then
 * selects these PLLs as the respective CPU/DSU clock sources.
 */
static void set_pll_cpux_axi(void)
{
	uint32_t reg_val;
	core_pll_freq_fact cpu_pll;

	writel(0x0305, SUNXI_CPU_SYS_CFG_BASE + 0x60U);
	writel(0x0305, SUNXI_CPU_SYS_CFG_BASE + 0x64U);
	sdelay(20);
	writel(0, SUNXI_CPU_SYS_CFG_BASE + 0x6cU);
	sdelay(20);

	cpu_pll.FactorM0 = 0;
	cpu_pll.FactorN = 0x2a; /*1008M*/
	cpu_pll.FactorM1 = 0;
	cpu_pll.FactorP = 0;
	enable_pll(SUNXI_CPU_SYS_CFG_BASE + 0x04U, &cpu_pll, 0x48801400);

	cpu_pll.FactorM0 = 0;
	cpu_pll.FactorN = 0x2a; /*1008M*/
	cpu_pll.FactorM1 = 0;
	cpu_pll.FactorP = 0;
	enable_pll(SUNXI_CPU_SYS_CFG_BASE + 0x0cU, &cpu_pll, 0x48801400);

	cpu_pll.FactorM0 = 0;
	cpu_pll.FactorN = 0x1d; /*696M*/
	cpu_pll.FactorM1 = 0;
	cpu_pll.FactorP = 0;
	enable_pll(SUNXI_CPU_SYS_CFG_BASE + 0x08U, &cpu_pll, 0x48801400);

	/* PLL_CPU1 is core0~core3 clock,  select PLL_CPU1  clock src:
	 * PLL_CPU1/P ,P = 1,  */
	/*set and change cpu clk src to PLL_CPU1,  PLL_CPU1/P*/
	reg_val = readl(SUNXI_CPU_SYS_CFG_BASE + 0x60U);
	reg_val &= ~(0x03 << 24);
	reg_val |= (0x03 << 24);
	reg_val &= ~(0x01 << 16); // P = 1
	reg_val |= (0x00 << 16);
	writel(reg_val, SUNXI_CPU_SYS_CFG_BASE + 0x60U);
	sdelay(20);

	/* PLL_CPU3 is core4~core7 clock,  select PLL_CPU3  clock src:
	 * PLL_CPU3/P ,P = 1,  */
	/*set and change cpu clk src to PLL_CPU1,  PLL_CPU1/P*/
	reg_val = readl(SUNXI_CPU_SYS_CFG_BASE + 0x64U);
	reg_val &= ~(0x03 << 24);
	reg_val |= (0x03 << 24);
	reg_val &= ~(0x01 << 16); // P = 1
	reg_val |= (0x00 << 16);
	writel(reg_val, SUNXI_CPU_SYS_CFG_BASE + 0x64U);
	sdelay(20);

	/*dsu clk src to PLL_CPU2,  PLL_CPU2/P*/
	reg_val = readl(SUNXI_CPU_SYS_CFG_BASE + 0x6cU);
	reg_val &= ~(0x03 << 24);
	reg_val |= (0x03 << 24);
	reg_val &= ~(0x01 << 16); // P = 1
	reg_val |= (0x00 << 16);
	writel(reg_val, SUNXI_CPU_SYS_CFG_BASE + 0x6cU);
	sdelay(20);
}

/**
 * @brief Enable the PERIPH0 PLL.
 *
 * Programs the default PLL factors, enables lock and the PLL, and waits for
 * the PLL to lock. If FEL has already enabled the PLL this is a no-op.
 */
static void set_pll_periph0(void)
{
	uint32_t reg_val;

	if ((1U << 31) & readl(SUNXI_CCMU_BASE + CCU_PLL_PERI0_CTRL_REG)) {
		/*fel has enable pll_periph0*/
		pr_debug("periph0 has been enabled\n");
		return;
	}

	/* set default val  24MHz * N/M1/P0 = 24 * 100 /1/2 = 1.2G*/
	writel((0x48216310), SUNXI_CCMU_BASE + CCU_PLL_PERI0_CTRL_REG);

	/* lock enable */
	reg_val = readl(SUNXI_CCMU_BASE + CCU_PLL_PERI0_CTRL_REG);
	reg_val |= (1 << 29);
	writel(reg_val, SUNXI_CCMU_BASE + CCU_PLL_PERI0_CTRL_REG);

	/* lock enable */
	reg_val = readl(SUNXI_CCMU_BASE + CCU_PLL_PERI0_CTRL_REG);
	reg_val |= (1 << 30);
	writel(reg_val, SUNXI_CCMU_BASE + CCU_PLL_PERI0_CTRL_REG);

	/* enabe PLL: 600M(1X)  1200M(2x) 2400M(4X) */
	reg_val = readl(SUNXI_CCMU_BASE + CCU_PLL_PERI0_CTRL_REG);
	reg_val |= (1 << 31);
	writel(reg_val, SUNXI_CCMU_BASE + CCU_PLL_PERI0_CTRL_REG);

	while (!(readl(SUNXI_CCMU_BASE + CCU_PLL_PERI0_CTRL_REG) & (0x1 << 28)))
		;
	sdelay(20);
	/* lock disable */
	reg_val = readl(SUNXI_CCMU_BASE + CCU_PLL_PERI0_CTRL_REG);
	reg_val &= (~(1 << 29));
	writel(reg_val, SUNXI_CCMU_BASE + CCU_PLL_PERI0_CTRL_REG);
}

/**
 * @brief Enable the PERIPH1 PLL.
 *
 * Programs the default PLL factors, enables lock and the PLL, and waits for
 * the PLL to lock. If FEL has already enabled the PLL this is a no-op.
 */
static void set_pll_periph1(void)
{
	uint32_t reg_val;

	if ((1U << 31) & readl(SUNXI_CCMU_BASE + CCU_PLL_PERI1_CTRL_REG)) {
		/*fel has enable pll_periph0*/
		pr_debug("periph0 has been enabled\n");
		return;
	}

	/* set default val  24MHz * N/M1/P0 = 24 * 100 /1/2 = 1.2G*/
	writel((0x48216310), SUNXI_CCMU_BASE + CCU_PLL_PERI1_CTRL_REG);

	/* lock enable */
	reg_val = readl(SUNXI_CCMU_BASE + CCU_PLL_PERI1_CTRL_REG);
	reg_val |= (1 << 29);
	writel(reg_val, SUNXI_CCMU_BASE + CCU_PLL_PERI1_CTRL_REG);

	/* lock enable */
	reg_val = readl(SUNXI_CCMU_BASE + CCU_PLL_PERI1_CTRL_REG);
	reg_val |= (1 << 30);
	writel(reg_val, SUNXI_CCMU_BASE + CCU_PLL_PERI1_CTRL_REG);

	/* enabe PLL: 600M(1X)  1200M(2x) 2400M(4X) */
	reg_val = readl(SUNXI_CCMU_BASE + CCU_PLL_PERI1_CTRL_REG);
	reg_val |= (1 << 31);
	writel(reg_val, SUNXI_CCMU_BASE + CCU_PLL_PERI1_CTRL_REG);

	while (!(readl(SUNXI_CCMU_BASE + CCU_PLL_PERI1_CTRL_REG) & (0x1 << 28)))
		;
	sdelay(20);
	/* lock disable */
	reg_val = readl(SUNXI_CCMU_BASE + CCU_PLL_PERI1_CTRL_REG);
	reg_val &= (~(1 << 29));
	writel(reg_val, SUNXI_CCMU_BASE + CCU_PLL_PERI1_CTRL_REG);
}

/**
 * @brief Configure the AHB0 bus clock.
 *
 * Sets the AHB0 divider to derive 200 MHz from the 600 MHz PLL.
 */
static void set_ahb(void)
{
	/* PLL6:AHB1:APB1 = 600M:200M  AHB = src clk/M */
	writel((2 << 0), SUNXI_CCMU_BASE + CCU_AHB0_CFG_REG);
	writel((0x03 << 24) | readl(SUNXI_CCMU_BASE + CCU_AHB0_CFG_REG), SUNXI_CCMU_BASE + CCU_AHB0_CFG_REG);
	sdelay(1);
}

/**
 * @brief Configure the APB0 bus clock.
 *
 * Sets the APB0 divider to derive 100 MHz from the 600 MHz PLL.
 */
static void set_apb(void)
{
	/*PLL6:APB0 = 600M:100M  APB = src clk/M */
	writel((5 << 0), SUNXI_CCMU_BASE + CCU_APB0_CFG_REG);
	writel((0x03 << 24) | readl(SUNXI_CCMU_BASE + CCU_APB0_CFG_REG), SUNXI_CCMU_BASE + CCU_APB0_CFG_REG);
	sdelay(1);
}

/**
 * @brief Deassert the DMA reset and open the DMA clock gate.
 *
 * Releases the DMA from reset and enables the gating clock so the DMA engine
 * can be used by the boot loader.
 */
static void set_pll_dma(void)
{
	/*dma reset*/
	writel(readl(SUNXI_CCMU_BASE + CCU_DMA_BGR_REG) | (1 << 16), SUNXI_CCMU_BASE + CCU_DMA_BGR_REG);
	sdelay(20);
	/*gating clock for dma pass*/
	writel(readl(SUNXI_CCMU_BASE + CCU_DMA_BGR_REG) | (1 << 0), SUNXI_CCMU_BASE + CCU_DMA_BGR_REG);
}

/**
 * @brief Configure the MBUS clock.
 *
 * Resets the MBUS domain, sets the divider to derive 466 MHz from the DDR
 * PLL, selects that source and opens the MBUS clock gate.
 */
static void set_pll_mbus(void)
{
	uint32_t reg_val = 0;

	/*reset mbus domain*/
	reg_val |= (0x1 << 30);
	writel(reg_val, SUNXI_CCMU_BASE + CCU_MBUS_CFG_REG);
	sdelay(1);

	/* set MBUS div   m = 1*/
	/* set MBUS clock source to pllddr, mbus=pllddr/(m+1) = 933*2/4 = 466M */
	reg_val |= (0x0 << 24);
	reg_val |= 0x3;
	/* open MBUS clock */
	reg_val |= (0X01 << 31);
	/* set mbus upd bit*/
	reg_val |= (0x1 << 27);
	writel(reg_val, SUNXI_CCMU_BASE + CCU_MBUS_CFG_REG);
	sdelay(1);
}

/**
 * @brief Enable the analog calibration circuits.
 *
 * Powers on the analog calibration block used by the ADC and other analog
 * peripherals.
 */
static void set_circuits_analog(void)
{
	/* calibration circuits analog enable */
	setbits_le32(SUNXI_R_PRCM_BASE + 0x250U, 0x01 << VDD_ADDA_OFF_GATING);
	sdelay(1);

	setbits_le32(SUNXI_R_PRCM_BASE + 0x310U, 0x01 << CAL_ANA_EN);
	sdelay(1);

	clrbits_le32(SUNXI_R_PRCM_BASE + 0x310U, 0x01 << CAL_EN);
	sdelay(1);

	setbits_le32(SUNXI_R_PRCM_BASE + 0x310U, 0x01 << CAL_EN);
	sdelay(1);
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
	writel(0x01, SUNXI_CCMU_BASE + CCU_IOMMU_BGR_REG);
	/*enable auto gating*/
	writel(0x01, SUNXI_IOMMU_BASE + 0x40U);
}

/**
 * @brief Apply the platform-specific clock configuration.
 *
 * Enables the analog calibration circuits and the IOMMU auto gating.
 */
static void set_platform_config(void)
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
static void set_modules_clock(void)
{
	uint32_t reg_val, i;
	uintptr_t ccmu_pll_addr[] = {
		SUNXI_CCMU_BASE + CCU_PLL_PERI0_CTRL_REG,  SUNXI_CCMU_BASE + CCU_PLL_PERI1_CTRL_REG,  SUNXI_CCMU_BASE + CCU_PLL_GPU_CTRL_REG,
		SUNXI_CCMU_BASE + CCU_PLL_VIDE00_CTRL_REG, SUNXI_CCMU_BASE + CCU_PLL_VIDE01_CTRL_REG, SUNXI_CCMU_BASE + CCU_PLL_VIDE02_CTRL_REG,
		SUNXI_CCMU_BASE + CCU_PLL_VIDE03_CTRL_REG, SUNXI_CCMU_BASE + CCU_PLL_VE_CTRL_REG,     SUNXI_CCMU_BASE + CCU_PLL_AUDIO_CTRL_REG,
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
 * Applies the platform configuration, then programs the CPU PLLs, PERIPH0/1
 * PLLs, AHB/APB dividers, DMA, MBUS and module clocks.
 */
void sunxi_clk_init(void)
{
	pr_debug("Set SoC 1890 (A523/A527/MR527/T527) CLK Start.\n");
	set_platform_config();
	set_pll_cpux_axi();
	set_pll_periph0();
	set_pll_periph1();
	set_ahb();
	set_apb();
	set_pll_dma();
	set_pll_mbus();
	set_modules_clock();
	pr_debug("Set pll done\n");
	return;
}

/**
 * @brief Reset the AHB/APB and CPUX clocks to their default state.
 *
 * Switches AHB0 and APB0 back to the OSC24M source and restores the default
 * CPU clock configuration register.
 */
void sunxi_clk_reset(void)
{
	uint32_t reg_val;
	/*set ahb,apb to default, use OSC24M*/
	reg_val = readl(SUNXI_CCMU_BASE + CCU_AHB0_CFG_REG);
	reg_val &= (~(0x3 << 24));
	writel(reg_val, SUNXI_CCMU_BASE + CCU_AHB0_CFG_REG);

	reg_val = readl(SUNXI_CCMU_BASE + CCU_APB0_CFG_REG);
	reg_val &= (~(0x3 << 24));
	writel(reg_val, SUNXI_CCMU_BASE + CCU_APB0_CFG_REG);

	/*set cpux pll to default,use OSC24M*/
	writel(0x0305, SUNXI_CPU_SYS_CFG_BASE + 0x60U);
	return;
}

/**
 * @brief Set the CPU PLL to a given frequency.
 *
 * Switches the CPU clock to OSC24M, re-tunes PLL_CPU1 for the requested
 * frequency and PLL_CPU2 for 936 MHz, then selects PLL_CPU1/P and PLL_CPU2/P
 * as the core and DSU clock sources.
 *
 * @param[in] freq Desired CPU frequency in MHz.
 */
void sun55iw3_clk_set_cpu_pll(uint32_t freq)
{
	uint32_t reg_val = 0;
	core_pll_freq_fact cpu_pll;

	/* switch to 24M*/
	writel(0x0305, SUNXI_CPU_SYS_CFG_BASE + 0x60U);
	udelay(20);
	writel(0, SUNXI_CPU_SYS_CFG_BASE + 0x6cU);
	udelay(20);

	cpu_pll.FactorM0 = 0;
	cpu_pll.FactorN = freq / 24;
	cpu_pll.FactorM1 = 0;
	cpu_pll.FactorP = 0;
	enable_pll(SUNXI_CPU_SYS_CFG_BASE + 0x04U, &cpu_pll, 0x48801400);

	cpu_pll.FactorM0 = 0;
	cpu_pll.FactorN = 0x27; /*936M*/
	cpu_pll.FactorM1 = 0;
	cpu_pll.FactorP = 0;
	enable_pll(SUNXI_CPU_SYS_CFG_BASE + 0x08U, &cpu_pll, 0x48801400);

	/* PLL_CPU1 is core0~core3 clock,  select PLL_CPU1  clock src:
	 * PLL_CPU1/P ,P = 1,  */
	/*set and change cpu clk src to PLL_CPU1,  PLL_CPU1/P*/
	reg_val = readl(SUNXI_CPU_SYS_CFG_BASE + 0x60U);
	reg_val &= ~(0x03 << 24);
	reg_val |= (0x03 << 24);
	reg_val &= ~(0x01 << 16); // P = 1
	reg_val |= (0x00 << 16);
	writel(reg_val, SUNXI_CPU_SYS_CFG_BASE + 0x60U);
	sdelay(20);

	/*dsu clk src to PLL_CPU2,  PLL_CPU2/P*/
	reg_val = readl(SUNXI_CPU_SYS_CFG_BASE + 0x6cU);
	reg_val &= ~(0x03 << 24);
	reg_val |= (0x03 << 24);
	reg_val &= ~(0x01 << 16); // P = 1
	reg_val |= (0x00 << 16);
	writel(reg_val, SUNXI_CPU_SYS_CFG_BASE + 0x6cU);
	sdelay(20);
}

/**
 * @brief Dump the frequency of one CPU core clock.
 *
 * Resolves the clock source and prints the computed frequency of the given
 * CPU domain.
 *
 * @param[in] cpuid   CPU index used for the log output.
 * @param[in] cpu_reg PLL control register address of the CPU domain.
 */
static void sunxi_cpux_clk_dump(uint8_t cpuid, uintptr_t cpu_reg)
{
	uint32_t reg_val;
	uint32_t div_m, div_p, div_m1;
	uint32_t factor_n, factor_p;
	uint32_t clock, clock_src;

	reg_val = readl(SUNXI_CPU_SYS_CFG_BASE + 0x60U);
	clock_src = (reg_val >> 24) & 0x03;
	factor_p = 1 << ((reg_val >> 16) & 0x3);

	switch (clock_src) {
	case 0: //OSC24M
		clock = 24;
		break;
	case 1: //RTC32K
		clock = 32 / 1000;
		break;
	case 2: //RC16M
		clock = 16;
		break;
	case 3: //PLL_CPU1
		reg_val = read32(cpu_reg);
		div_p = ((reg_val >> 16) & 0xf) + 1;
		factor_n = ((reg_val >> 8) & 0xff);
		div_m = ((reg_val >> 0) & 0xf) + 1;
		div_m1 = ((reg_val >> 20) & 0x3) + 1;

		clock = 24 * factor_n / div_p / (div_m * div_m1);
		break;
	default:
		pr_debug("CPU CLK Disable\r\n");
	}
	pr_debug("CPU%d FREQ=%luMHz\r\n", cpuid, clock / factor_p);
}

/**
 * @brief Dump the current SoC clock configuration.
 *
 * Prints the CPU0..CPU3 frequencies and the computed PERIPH0/PERIPH1, DDR0/
 * DDR1 and HSIC PLL frequencies.
 */
void sunxi_clk_dump(void)
{
	uint32_t reg32;
	uint32_t plln, pllm;
	uint8_t p0;
	uint8_t p1;

	sunxi_cpux_clk_dump(0, SUNXI_CPU_SYS_CFG_BASE + 0x00U);
	sunxi_cpux_clk_dump(1, SUNXI_CPU_SYS_CFG_BASE + 0x04U);
	sunxi_cpux_clk_dump(2, SUNXI_CPU_SYS_CFG_BASE + 0x08U);
	sunxi_cpux_clk_dump(3, SUNXI_CPU_SYS_CFG_BASE + 0x0cU);

	/* PLL PERIx */
	reg32 = read32(SUNXI_CCMU_BASE + CCU_PLL_PERI0_CTRL_REG);
	if (reg32 & (1 << 31)) {
		plln = ((reg32 >> 8) & 0xff) + 1;
		pllm = (reg32 & 0x01) + 1;
		p0 = ((reg32 >> 16) & 0x03) + 1;
		p1 = ((reg32 >> 20) & 0x03) + 1;

		pr_debug("PLL_PERI0 (2X)=%luMHz, (1X)=%luMHz, (800M)=%luMHz\r\n", (24 * plln) / (pllm * p0), (24 * plln) / (pllm * p0) >> 1, (24 * plln) / (pllm * p1));
	} else {
		pr_debug("PLL_PERI0 disabled\r\n");
	}

	/* PLL PERIx */
	reg32 = read32(SUNXI_CCMU_BASE + CCU_PLL_PERI1_CTRL_REG);
	if (reg32 & (1 << 31)) {
		plln = ((reg32 >> 8) & 0xff) + 1;
		pllm = (reg32 & 0x01) + 1;
		p0 = ((reg32 >> 16) & 0x03) + 1;
		p1 = ((reg32 >> 20) & 0x03) + 1;

		pr_debug("PLL_PERI1 (2X)=%luMHz, (1X)=%luMHz, (800M)=%luMHz\r\n", (24 * plln) / (pllm * p0), (24 * plln) / (pllm * p0) >> 1, (24 * plln) / (pllm * p1));
	} else {
		pr_debug("PLL_PERI1 disabled\r\n");
	}

	/* PLL DDR0 */
	reg32 = read32(SUNXI_CCMU_BASE + CCU_PLL_DDR0_CTRL_REG);
	if (reg32 & (1 << 31)) {
		plln = ((reg32 >> 8) & 0xff) + 1;

		pllm = (reg32 & 0x01) + 1;
		p1 = ((reg32 >> 1) & 0x1) + 1;
		p0 = (reg32 & 0x01) + 1;

		pr_debug("PLL_DDR0=%luMHz\r\n", (24 * plln) / (p0 * p1));

	} else {
		pr_debug("PLL_DDR0 disabled\r\n");
	}

	/* PLL DDR1 */
	reg32 = read32(SUNXI_CCMU_BASE + CCU_PLL_DDR1_CTRL_REG);
	if (reg32 & (1 << 31)) {
		plln = ((reg32 >> 8) & 0xff) + 1;

		pllm = (reg32 & 0x01) + 1;
		p1 = ((reg32 >> 1) & 0x1) + 1;
		p0 = (reg32 & 0x01) + 1;

		pr_debug("PLL_DDR1=%luMHz\r\n", (24 * plln) / (p0 * p1));

	} else {
		pr_debug("PLL_DDR1 disabled\r\n");
	}

	/* PLL HSIC */
	reg32 = read32(SUNXI_CCMU_BASE + CCU_PLL_HSIC_CTRL_REG);
	if (reg32 & (1 << 31)) {
		plln = ((reg32 >> 8) & 0xff) + 1;

		pllm = (reg32 & 0x01) + 1;
		p1 = ((reg32 >> 1) & 0x1) + 1;
		p0 = (reg32 & 0x01) + 1;

		pr_debug("HSIC=%luMHz\r\n", (24 * plln) / (p0 * p1));

	} else {
		pr_debug("HSIC disabled\r\n");
	}
}
