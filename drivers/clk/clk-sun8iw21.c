/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file clk-sun8iw21.c
 * @brief Clock driver for the Allwinner sun8iw21 SoC.
 *
 * Programs the CPU PLL and the AHB/APB0 dividers together with the DMA and
 * MBUS clocks during early boot, and provides clock dump and reset helpers.
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
#include <drivers/clk/sun8iw21/reg.h>

/**
 * @brief Initialize the SoC clocks.
 *
 * Programs the CPU PLL to 1200 MHz (N = 50) with an AXI divider of 2,
 * configures the AHB clock to 200 MHz and the APB0 clock to 100 MHz, then
 * releases the DMA and MBUS clock domains.
 */
void sunxi_clk_init(void)
{
	uint32_t val;

	/* cpu_clk = CPU_PLL/P, AXI_DIV = 2 */
	write32(CCU_BASE + CCU_CPU_CLK_REG, (0x3 << 8) | 0x01);
	sdelay(1);

	/* cpu_clk divider = 1 */
	val = read32(CCU_BASE + CCU_CPU_CLK_REG);
	val &= ~((1 << 16) | (1 << 17));
	write32(CCU_BASE + CCU_CPU_CLK_REG, val);
	sdelay(5);

	/* CPU_PLL: enable LOCK, enable LDO, N = 50 * 24 = 1200MHz */
	/* 
	 * 408000000Hz：val |= (1 << 30 | (1 << 29) | (17 << 8));
	 * 600000000Hz：val |= (1 << 30 | (1 << 29) | (25 << 8));
	 * 720000000Hz：val |= (1 << 30 | (1 << 29) | (30 << 8));
	 * 900000000Hz：val |= (1 << 30 | (1 << 29) | (37 << 8));
	 * 1008000000Hz：val |= (1 << 30 | (1 << 29) | (42 << 8));
	 * 1200000000Hz：val |= (1 << 30 | (1 << 29) | (50 << 8));
	 */
	val = read32(CCU_BASE + CCU_PLL_CPU_CTRL_REG);
	/* CPU_PLL: Output disable, PLL_N = 0, M = 0 */
	val &= ~((1 << 27) | (0x3FF << 8) | 0x3);
	val |= (1 << 30 | (1 << 29) | (37 << 8));
	write32(CCU_BASE + CCU_PLL_CPU_CTRL_REG, val);
	sdelay(5);

	/* wait for PLL lock */
	while (!(read32(CCU_BASE + CCU_PLL_CPU_CTRL_REG) & (0x1 << 28)))
		;

	sdelay(20);

	/* PLL lock disable, output enable */
	val = read32(CCU_BASE + CCU_PLL_CPU_CTRL_REG);
	val &= ~(1 << 29);
	val |= (1 << 27);
	write32(CCU_BASE + CCU_PLL_CPU_CTRL_REG, val);

	/* cpu clock = CPU_PLL / P, APB DIV = 4, AXI_DIV = 2 */
	val = read32(CCU_BASE + CCU_CPU_CLK_REG);
	val &= ~((0x7 << 24) | (0x3 << 8) | 0x3);
	val |= ((0x3 << 24) | (0x3 << 8) | 0x1);
	write32(CCU_BASE + CCU_CPU_CLK_REG, val);
	sdelay(1);

	/* Periph0 has been enabled */
	if (read32(CCU_BASE + CCU_PLL_PERI_CTRL_REG) & (1 << 31))
		pr_debug("periph0 has been enabled\n");

	/* AHB_Clock = CLK_SRC/M/N, PERIPH_600M / N(1) / M(3) = 200MHz */
	write32(CCU_BASE + CCU_AHB_CLK_REG, (0x3 << 24) | 0x2);
	sdelay(1);

	/* APB0_Clock = CLK_SRC/M/N, PERIPH_600M / N(2) / M(3) = 100MHz */
	write32(CCU_BASE + CCU_APB0_CLK_REG, (0x3 << 24) | (1 << 8) | 0x2);
	sdelay(1);

	/* DMA reset */
	val = read32(CCU_BASE + CCU_DMA_BGR_REG);
	val |= (1 << 16);
	write32(CCU_BASE + CCU_DMA_BGR_REG, val);
	sdelay(20);

	/* enable DMA gate */
	val = read32(CCU_BASE + CCU_DMA_BGR_REG);
	val |= 0x01;
	write32(CCU_BASE + CCU_DMA_BGR_REG, val);
	sdelay(1);

	/* MBUS reset */
	val = read32(CCU_BASE + CCU_MBUS_CLK_REG);
	val |= (1 << 30);
	write32(CCU_BASE + CCU_MBUS_CLK_REG, val);
	sdelay(1);

	pr_debug("sunxi clock init end\n");
	pr_debug("cpu clk reg (#0x%x): 0x%08x\n", CCU_CPU_CLK_REG, read32(CCU_BASE + CCU_CPU_CLK_REG));

	return;
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
	reg_val = read32(CCU_BASE + CCU_AHB_CLK_REG);
	reg_val &= (~((0x3 << 24) | (0x3 << 8) | (0x3)));
	write32(CCU_BASE + CCU_AHB_CLK_REG, reg_val);

	reg_val = read32(CCU_BASE + CCU_APB0_CLK_REG);
	reg_val &= (~((0x3 << 24) | (0x3 << 8) | (0x3)));
	write32(CCU_BASE + CCU_APB0_CLK_REG, reg_val);

	/*set cpux pll to default,use OSC24M*/
	write32(CCU_BASE + CCU_CPU_CLK_REG, 0x0301);
	return;
}

/**
 * @brief Dump the current SoC clock configuration.
 *
 * Prints the CPU PLL source and frequency, the PERI PLL frequencies and the
 * DDR PLL frequency.
 */
void sunxi_clk_dump(void)
{
	uint32_t reg32;
	uint32_t cpu_clk_src, plln, pllm;
	uint8_t p0, p1;
	const char *clock_str;

	/* PLL CPU */
	reg32 = read32(CCU_BASE + CCU_CPU_CLK_REG);
	cpu_clk_src = (reg32 >> 24) & 0x7;
	pr_debug("CLK: CPU CLK_reg=0x%08x\n", reg32);

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
		clock_str = "PLL_PERI_600M";
		break;

	case 0x5:
		clock_str = "PLL_PERI_800M";
		break;

	default:
		clock_str = "ERROR";
	}

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

	pr_debug("CLK: CPU PLL=%s FREQ=%uMHz\n", clock_str, ((((read32(CCU_BASE + CCU_PLL_CPU_CTRL_REG) >> 8) & 0xff) + 1) * 24 / p1));

	/* PLL PERI */
	reg32 = read32(CCU_BASE + CCU_PLL_PERI_CTRL_REG);
	if (reg32 & (1 << 31)) {
		plln = ((reg32 >> 8) & 0xff) + 1;
		pllm = (reg32 & 0x01) + 1;
		p0 = ((reg32 >> 16) & 0x03) + 1;
		p1 = ((reg32 >> 20) & 0x03) + 1;

		pr_debug("CLK: PLL_peri (2X)=%uMHz, (1X)=%uMHz, (800M)=%uMHz\n", (24 * plln) / (pllm * p0), (24 * plln) / (pllm * p0) >> 1, (24 * plln) / (pllm * p1));
	} else {
		pr_debug("CLK: PLL_peri disabled\n");
	}

	/* PLL DDR */
	reg32 = read32(CCU_BASE + CCU_PLL_DDR_CTRL_REG);
	if (reg32 & (1 << 31)) {
		plln = ((reg32 >> 8) & 0xff) + 1;

		pllm = (reg32 & 0x01) + 1;
		p1 = ((reg32 >> 1) & 0x1) + 1;
		p0 = (reg32 & 0x01) + 1;

		pr_debug("CLK: PLL_ddr=%uMHz\n", (24 * plln) / (p0 * p1));
	} else {
		pr_debug("CLK: PLL_ddr disabled\n");
	}
}
