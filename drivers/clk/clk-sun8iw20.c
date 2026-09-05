/* SPDX-License-Identifier: GPL-2.0+ */
#define pr_fmt(fmt) "clk-sun8iw20: " fmt

/**
 * @file clk-sun8iw20.c
 * @brief Clock driver for the Allwinner sun8iw20 SoC.
 *
 * Programs the CPU PLL and the PERI0 PLL together with the AHB/APB dividers,
 * DMA and MBUS clocks during early boot, and provides clock dump and reset
 * helpers.
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
#include <drivers/clk/sun8iw20/reg.h>

/**
 * @brief Configure the CPUX PLL and AXI clock.
 *
 * Switches the CPU/AXI clock source to the PERI PLL while the CPU PLL is
 * brought up, programs the CPU PLL to 1200 MHz (24 MHz * N / P, N = 50),
 * waits for the lock, then switches the CPU/AXI clock to the CPU PLL with an
 * AXI divider of 2 (1200 MHz : 600 MHz).
 */
static void set_pll_cpux_axi(void)
{
	uint32_t val;

	/* AXI: Select cpu clock src to PLL_PERI(1x) */
	write32(CCU_BASE + CCU_CPU_AXI_CFG_REG, (4 << 24) | (1 << 0));
	sdelay(10);

	/* Disable pll gating */
	val = read32(CCU_BASE + CCU_PLL_CPU_CTRL_REG);
	val &= ~(1 << 27);
	write32(CCU_BASE + CCU_PLL_CPU_CTRL_REG, val);

	/* Enable pll ldo */
	val = read32(CCU_BASE + CCU_PLL_CPU_CTRL_REG);
	val |= (1 << 30);
	write32(CCU_BASE + CCU_PLL_CPU_CTRL_REG, val);
	sdelay(5);

	/* Set clk to 1200 MHz */
	/* PLL_CPUX = 24 MHz*N/P */
	val = read32(CCU_BASE + CCU_PLL_CPU_CTRL_REG);
	val &= ~((0x3 << 16) | (0xff << 8) | (0x3 << 0));
	val |= (50 << 8);
	write32(CCU_BASE + CCU_PLL_CPU_CTRL_REG, val);

	/* Lock enable */
	val = read32(CCU_BASE + CCU_PLL_CPU_CTRL_REG);
	val |= (1 << 29);
	write32(CCU_BASE + CCU_PLL_CPU_CTRL_REG, val);

	/* Enable pll */
	val = read32(CCU_BASE + CCU_PLL_CPU_CTRL_REG);
	val |= (1 << 31);
	write32(CCU_BASE + CCU_PLL_CPU_CTRL_REG, val);

	/* Wait pll stable */
	while (!(read32(CCU_BASE + CCU_PLL_CPU_CTRL_REG) & (0x1 << 28)))
		;
	sdelay(20);

	/* Enable pll gating */
	val = read32(CCU_BASE + CCU_PLL_CPU_CTRL_REG);
	val |= (1 << 27);
	write32(CCU_BASE + CCU_PLL_CPU_CTRL_REG, val);

	/* Lock disable */
	val = read32(CCU_BASE + CCU_PLL_CPU_CTRL_REG);
	val &= ~(1 << 29);
	write32(CCU_BASE + CCU_PLL_CPU_CTRL_REG, val);
	sdelay(1);

	/* AXI: set and change cpu clk src to PLL_CPUX, PLL_CPUX:AXI0 = 1200MHz:600MHz */
	val = read32(CCU_BASE + CCU_CPU_AXI_CFG_REG);
	val &= ~(0x07 << 24 | 0x3 << 16 | 0x3 << 8 | 0xf << 0); // Clear
	val |= (0x03 << 24 | 0x0 << 16 | 0x1 << 8 | 0x1 << 0); // CLK_SEL=PLL_CPU/P, DIVP=0, DIV2=1, DIV1=1
	write32(CCU_BASE + CCU_CPU_AXI_CFG_REG, val);
	sdelay(1);
}

/**
 * @brief Enable the PERIPH0 PLL.
 *
 * Programs the PERI0 PLL for 600 MHz (1x) / 1200 MHz (2x) and waits for it to
 * lock. If the PLL was already enabled this is a no-op.
 */
static void set_pll_periph0(void)
{
	uint32_t val;

	/* Periph0 has been enabled */
	if (read32(CCU_BASE + CCU_PLL_PERI0_CTRL_REG) & (1 << 31))
		return;

	/* Set default val */
	write32(CCU_BASE + CCU_PLL_PERI0_CTRL_REG, 0x63 << 8);

	/* Lock enable */
	val = read32(CCU_BASE + CCU_PLL_PERI0_CTRL_REG);
	val |= (1 << 29);
	write32(CCU_BASE + CCU_PLL_PERI0_CTRL_REG, val);

	/* Enabe pll 600m(1x) 1200m(2x) */
	val = read32(CCU_BASE + CCU_PLL_PERI0_CTRL_REG);
	val |= (1 << 31);
	write32(CCU_BASE + CCU_PLL_PERI0_CTRL_REG, val);

	/* Wait pll stable */
	while (!(read32(CCU_BASE + CCU_PLL_PERI0_CTRL_REG) & (0x1 << 28)))
		;
	sdelay(20);

	/* Lock disable */
	val = read32(CCU_BASE + CCU_PLL_PERI0_CTRL_REG);
	val &= ~(1 << 29);
	write32(CCU_BASE + CCU_PLL_PERI0_CTRL_REG, val);
}

/**
 * @brief Configure the AHB (PSI) bus clock.
 *
 * Selects the PERI PLL as the AHB clock source with the programmed divider.
 */
static void set_ahb(void)
{
	write32(CCU_BASE + CCU_PSI_CLK_REG, (2 << 0) | (0 << 8) | (0x03 << 24));
	sdelay(1);
}

/**
 * @brief Configure the APB0 bus clock.
 *
 * Selects the PERI PLL as the APB0 clock source with the programmed divider.
 */
static void set_apb(void)
{
	write32(CCU_BASE + CCU_APB0_CLK_REG, (2 << 0) | (1 << 8) | (0x03 << 24));
	sdelay(1);
}

/**
 * @brief Configure the DMA clock.
 *
 * Deasserts the DMA reset and opens the gating clock so the DMA engine can be
 * used by the boot loader.
 */
static void set_dma(void)
{
	/* Dma reset */
	write32(CCU_BASE + CCU_DMA_BGR_REG, read32(CCU_BASE + CCU_DMA_BGR_REG) | (1 << 16));
	sdelay(20);
	/* Enable gating clock for dma */
	write32(CCU_BASE + CCU_DMA_BGR_REG, read32(CCU_BASE + CCU_DMA_BGR_REG) | (1 << 0));
}

/**
 * @brief Configure the MBUS clock.
 *
 * Resets the MBUS clock domain and enables the MBUS master clock gating.
 */
static void set_mbus(void)
{
	uint32_t val;

	/* Reset mbus domain */
	val = read32(CCU_BASE + CCU_MBUS_CLK_REG);
	val |= (0x1 << 30);
	write32(CCU_BASE + CCU_MBUS_CLK_REG, val);
	sdelay(1);

	/* Enable mbus master clock gating */
	write32(CCU_BASE + CCU_MBUS_MAT_CLK_GATING_REG, 0x00000d87);
}

/**
 * @brief Enable a module PLL.
 *
 * Enables the PLL, LDO and lock action for the PLL control register at the
 * given address and waits for it to lock. PLLs that are already enabled are
 * skipped.
 *
 * @param[in] addr Address of the module PLL control register.
 */
static void set_module(virtual_addr_t addr)
{
	uint32_t val;

	if (!(read32(addr) & (1 << 31))) {
		val = read32(addr);
		write32(addr, val | (1 << 31) | (1 << 30));

		/* Lock enable */
		val = read32(addr);
		val |= (1 << 29);
		write32(addr, val);

		/* Wait pll stable */
		while (!(read32(addr) & (0x1 << 28)))
			;
		sdelay(20);

		/* Lock disable */
		val = read32(addr);
		val &= ~(1 << 29);
		write32(addr, val);
	}
}

/**
 * @brief Initialize the SoC clocks.
 *
 * Programs the CPUX and PERIPH0 PLLs, configures the AHB/APB dividers and the
 * DMA/MBUS clocks, then enables the VIDEO, VE and AUDIO module PLLs.
 */
void sunxi_clk_init(void)
{
	set_pll_cpux_axi();
	set_pll_periph0();
	set_ahb();
	set_apb();
	set_dma();
	set_mbus();
	//set_module(CCU_BASE + CCU_PLL_PERI0_CTRL_REG);
	set_module(CCU_BASE + CCU_PLL_VIDEO0_CTRL_REG);
	set_module(CCU_BASE + CCU_PLL_VIDEO1_CTRL_REG);
	set_module(CCU_BASE + CCU_PLL_VE_CTRL);
	set_module(CCU_BASE + CCU_PLL_AUDIO0_CTRL_REG);
	set_module(CCU_BASE + CCU_PLL_AUDIO1_CTRL_REG);
}

/**
 * @brief Reset the SoC clocks to their default state.
 *
 * Switches the AHB, APB0 and CPUX clocks back to their default OSC24M
 * configuration.
 */
void sunxi_clk_reset(void)
{
	uint32_t reg_val;

	/*set ahb,apb to default, use OSC24M*/
	reg_val = readl(CCU_BASE + CCU_PSI_CLK_REG);
	reg_val &= (~((0x3 << 24) | (0x3 << 8) | (0x3)));
	writel(reg_val, CCU_BASE + CCU_PSI_CLK_REG);

	reg_val = readl(CCU_BASE + CCU_APB0_CLK_REG);
	reg_val &= (~((0x3 << 24) | (0x3 << 8) | (0x3)));
	writel(reg_val, CCU_BASE + CCU_APB0_CLK_REG);

	/*set cpux pll to default,use OSC24M*/
	writel(0x0301, CCU_BASE + CCU_CPU_AXI_CFG_REG);
	return;
}

/**
 * @brief Dump the current SoC clock configuration.
 *
 * Prints the CPU PLL source and frequency, the PERI0 PLL frequencies and the
 * DDR PLL frequency.
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
	reg32 = read32(CCU_BASE + CCU_CPU_AXI_CFG_REG);
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
		clock_str = "PLL_PERI(1X)";
		break;

	case 0x5:
		clock_str = "PLL_PERI(2X)";
		break;

	case 0x6:
		clock_str = "PLL_PERI(800M)";
		break;

	default:
		clock_str = "ERROR";
	}

	reg32 = read32(CCU_BASE + CCU_PLL_CPU_CTRL_REG);
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

	pr_debug("CPU PLL=%s FREQ=%luMHz\r\n", clock_str, ((((reg32 >> 8) & 0xff) + 1) * 24 / p1));

	/* PLL PERIx */
	reg32 = read32(CCU_BASE + CCU_PLL_PERI0_CTRL_REG);
	if (reg32 & (1 << 31)) {
		plln = ((reg32 >> 8) & 0xff) + 1;
		pllm = (reg32 & 0x01) + 1;
		p0 = ((reg32 >> 16) & 0x03) + 1;
		p1 = ((reg32 >> 20) & 0x03) + 1;

		pr_debug("PLL_peri (2X)=%luMHz, (1X)=%luMHz, (800M)=%luMHz\r\n", (24 * plln) / (pllm * p0), (24 * plln) / (pllm * p0) >> 1, (24 * plln) / (pllm * p1));
	} else {
		pr_debug("PLL_peri disabled\r\n");
	}

	/* PLL DDR */
	reg32 = read32(CCU_BASE + CCU_PLL_DDR_CTRL_REG);
	if (reg32 & (1 << 31)) {
		plln = ((reg32 >> 8) & 0xff) + 1;

		pllm = (reg32 & 0x01) + 1;
		p1 = ((reg32 >> 1) & 0x1) + 1;
		p0 = (reg32 & 0x01) + 1;

		pr_debug("PLL_ddr=%luMHz\r\n", (24 * plln) / (p0 * p1));

	} else {
		pr_debug("PLL_ddr disabled\r\n");
	}
}
