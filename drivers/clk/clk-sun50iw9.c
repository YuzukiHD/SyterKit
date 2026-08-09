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

static inline void set_pll_cpux_axi(sunxi_ccu_t *ccu) {
	uint32_t reg_val;
	/* select CPUX clock src: OSC24M, AXI divide ratio is 2, system apb clk ratio is 4 */
	writel((0 << 24) | (3 << 8) | (1 << 0), sunxi_ccu_reg(ccu, CCU_CPUX_AXI_CFG_REG));
	sdelay(1);

	/* disable pll */
	reg_val = read32(sunxi_ccu_reg(ccu, CCU_PLL_CPUX_CTRL_REG)) & ~(1 << 31);
	writel(reg_val, sunxi_ccu_reg(ccu, CCU_PLL_CPUX_CTRL_REG));

	/* set default val: clk is 1008M, PLL_OUTPUT = 24 M*N / (M*P) */
	reg_val = (read32(sunxi_ccu_reg(ccu, CCU_PLL_CPUX_CTRL_REG)) & ~((0x3 << 16) | (0xff << 8) | (0x3 << 0))) | (41 << 8);
	writel(reg_val, sunxi_ccu_reg(ccu, CCU_PLL_CPUX_CTRL_REG));

	/* lock enable */
	reg_val = read32(sunxi_ccu_reg(ccu, CCU_PLL_CPUX_CTRL_REG)) | (1 << 29);
	writel(reg_val, sunxi_ccu_reg(ccu, CCU_PLL_CPUX_CTRL_REG));

	/* enable pll */
	reg_val = read32(sunxi_ccu_reg(ccu, CCU_PLL_CPUX_CTRL_REG)) | (1 << 31);
	writel(reg_val, sunxi_ccu_reg(ccu, CCU_PLL_CPUX_CTRL_REG));

	/* wait PLL_CPUX stable */
	while (!(read32(sunxi_ccu_reg(ccu, CCU_PLL_CPUX_CTRL_REG)) & (0x1 << 28)))
		;
	sdelay(20);

	/* lock disable */
	reg_val = read32(sunxi_ccu_reg(ccu, CCU_PLL_CPUX_CTRL_REG)) & ~(1 << 29);
	writel(reg_val, sunxi_ccu_reg(ccu, CCU_PLL_CPUX_CTRL_REG));

	/* set and change cpu clk src to PLL_CPUX, PLL_CPUX:AXI0 = 408M:136M */
	reg_val = (read32(sunxi_ccu_reg(ccu, CCU_CPUX_AXI_CFG_REG)) & ~(0x03 << 24)) | (0x03 << 24);
	writel(reg_val, sunxi_ccu_reg(ccu, CCU_CPUX_AXI_CFG_REG));
	sdelay(1);
}

static inline void set_pll_periph0(sunxi_ccu_t *ccu) {
	uint32_t reg_val;

	if ((1U << 31) & read32(sunxi_ccu_reg(ccu, CCU_PLL_PERI0_CTRL_REG))) {
		//fel has enable pll_periph0
		printk_debug("periph0 has been enabled\n");
		return;
	}

	/* set default val*/
	writel(0x63 << 8, sunxi_ccu_reg(ccu, CCU_PLL_PERI0_CTRL_REG));

	/* lock enable */
	reg_val = read32(sunxi_ccu_reg(ccu, CCU_PLL_PERI0_CTRL_REG));
	reg_val |= (1 << 29);
	writel(reg_val, sunxi_ccu_reg(ccu, CCU_PLL_PERI0_CTRL_REG));

	/* enabe PLL: 600M(1X)  1200M(2x) 2400M(4X) */
	reg_val = read32(sunxi_ccu_reg(ccu, CCU_PLL_PERI0_CTRL_REG));
	reg_val |= (1 << 31);
	writel(reg_val, sunxi_ccu_reg(ccu, CCU_PLL_PERI0_CTRL_REG));

	/* lock disable */
	reg_val = read32(sunxi_ccu_reg(ccu, CCU_PLL_PERI0_CTRL_REG));
	reg_val &= (~(1 << 29));
	writel(reg_val, sunxi_ccu_reg(ccu, CCU_PLL_PERI0_CTRL_REG));
}

static inline void set_ahb(sunxi_ccu_t *ccu) {
	/* PLL6:AHB1:APB1 = 600M:200M:100M */
	writel((2 << 0) | (0 << 8), sunxi_ccu_reg(ccu, CCU_PSI_AHB1_AHB2_CFG_REG));
	writel((0x03 << 24) | read32(sunxi_ccu_reg(ccu, CCU_PSI_AHB1_AHB2_CFG_REG)), sunxi_ccu_reg(ccu, CCU_PSI_AHB1_AHB2_CFG_REG));
	sdelay(1);
	/*PLL6:AHB3 = 600M:200M*/
	writel((2 << 0) | (0 << 8), sunxi_ccu_reg(ccu, CCU_AHB3_CFG_GREG));
	writel((0x03 << 24) | read32(sunxi_ccu_reg(ccu, CCU_AHB3_CFG_GREG)), sunxi_ccu_reg(ccu, CCU_AHB3_CFG_GREG));
}

static inline void set_apb(sunxi_ccu_t *ccu) {
	/*PLL6:APB1 = 600M:100M */
	writel((2 << 0) | (1 << 8), sunxi_ccu_reg(ccu, CCU_APB1_CFG_GREG));
	writel((0x03 << 24) | read32(sunxi_ccu_reg(ccu, CCU_APB1_CFG_GREG)), sunxi_ccu_reg(ccu, CCU_APB1_CFG_GREG));
	sdelay(1);
}

static inline void set_pll_dma(sunxi_ccu_t *ccu) {
	/*dma reset*/
	writel(read32(sunxi_ccu_reg(ccu, CCU_DMA_BGR_REG)) | (1 << 16), sunxi_ccu_reg(ccu, CCU_DMA_BGR_REG));
	sdelay(20);
	/*gating clock for dma pass*/
	writel(read32(sunxi_ccu_reg(ccu, CCU_DMA_BGR_REG)) | (1 << 0), sunxi_ccu_reg(ccu, CCU_DMA_BGR_REG));
}

static inline void set_pll_mbus(sunxi_ccu_t *ccu) {
	uint32_t reg_val;

	/*reset mbus domain*/
	reg_val = 1 << 30;
	writel(1 << 30, sunxi_ccu_reg(ccu, CCU_MBUS_CFG_REG));
	sdelay(1);

	/* set MBUS div */
	reg_val = read32(sunxi_ccu_reg(ccu, CCU_MBUS_CFG_REG));
	reg_val |= (2 << 0);
	writel(reg_val, sunxi_ccu_reg(ccu, CCU_MBUS_CFG_REG));
	sdelay(1);

	/* set MBUS clock source to pll6(2x), mbus=pll6/(m+1) = 400M*/
	reg_val = read32(sunxi_ccu_reg(ccu, CCU_MBUS_CFG_REG));
	reg_val |= (1 << 24);
	writel(reg_val, sunxi_ccu_reg(ccu, CCU_MBUS_CFG_REG));
	sdelay(1);

	/* open MBUS clock */
	reg_val = read32(sunxi_ccu_reg(ccu, CCU_MBUS_CFG_REG));
	reg_val |= (0X01 << 31);
	writel(reg_val, sunxi_ccu_reg(ccu, CCU_MBUS_CFG_REG));
	sdelay(1);
}

static inline void set_circuits_analog(sunxi_ccu_t *ccu) {
	/* calibration circuits analog enable */
	uint32_t reg_val;

	reg_val = read32(sunxi_ccu_r_prcm_reg(ccu, 0x250U));
	reg_val |= (0x01 << VDD_ADDA_OFF_GATING);
	writel(reg_val, sunxi_ccu_r_prcm_reg(ccu, 0x250U));
	sdelay(1);

	reg_val = read32(sunxi_ccu_r_prcm_reg(ccu, 0x310U));
	reg_val |= (0x01 << CAL_ANA_EN);
	writel(reg_val, sunxi_ccu_r_prcm_reg(ccu, 0x310U));
	sdelay(1);

	reg_val = read32(sunxi_ccu_r_prcm_reg(ccu, 0x310U));
	reg_val &= ~(0x01 << CAL_EN);
	writel(reg_val, sunxi_ccu_r_prcm_reg(ccu, 0x310U));
	sdelay(1);

	reg_val = read32(sunxi_ccu_r_prcm_reg(ccu, 0x310U));
	reg_val |= (0x01 << CAL_EN);
	writel(reg_val, sunxi_ccu_r_prcm_reg(ccu, 0x310U));
	sdelay(1);
}

static inline void set_iommu_auto_gating(sunxi_ccu_t *ccu) {
	/*gating clock for iommu*/
	writel(0x01, sunxi_ccu_reg(ccu, CCU_IOMMU_BGR_REG));
	/*enable auto gating*/
	writel(0x01, sunxi_ccu_iommu_reg(ccu, 0x40U));
}

static inline void set_platform_config(sunxi_ccu_t *ccu) {
	/* 
     * At present, the audio codec finds a problem. VRA1 does not accelerate the power-on,
	 * which will affect the stability of the bais circuit and affect the boot speed.
	 * Therefore, the river vddon needs to be set to 1.
	*/
	set_circuits_analog(ccu);
	set_iommu_auto_gating(ccu);
}

static inline void set_modules_clock(sunxi_ccu_t *ccu) {
	uint32_t reg_val = 0x0;
	const uintptr_t modules_reg_addrs[] = {
			sunxi_ccu_reg(ccu, 0x28U),// peri1 clk
			sunxi_ccu_reg(ccu, 0x30U),// gpu clk
			sunxi_ccu_reg(ccu, 0x40U),// video0 clk
			sunxi_ccu_reg(ccu, 0x48U),// video1 clk
			sunxi_ccu_reg(ccu, 0x50U),// video2 clk
			sunxi_ccu_reg(ccu, 0x58U),// ve clk
			sunxi_ccu_reg(ccu, 0x60U),// de clk
			sunxi_ccu_reg(ccu, 0xe0U),// csi clk
			sunxi_ccu_reg(ccu, 0x78U) // audio clk
	};

	for (int i = 0; i < sizeof(modules_reg_addrs) / sizeof(modules_reg_addrs[0]); i++) {
		reg_val = read32(modules_reg_addrs[i]);
		reg_val |= (1 << 31);
		writel(reg_val, modules_reg_addrs[i]);
		sdelay(10);
	}
}

static inline int sunxi_clock_init_gpadc(sunxi_ccu_t *ccu) {
	uint32_t reg_val = 0;
	/* reset */
	reg_val = read32(sunxi_ccu_reg(ccu, CCU_GPADC_BGR_REG));
	reg_val &= ~(1 << 16);
	writel(reg_val, sunxi_ccu_reg(ccu, CCU_GPADC_BGR_REG));

	sdelay(2);

	reg_val |= (1 << 16);
	writel(reg_val, sunxi_ccu_reg(ccu, CCU_GPADC_BGR_REG));
	/* enable KEYADC gating */
	reg_val = read32(sunxi_ccu_reg(ccu, CCU_GPADC_BGR_REG));
	reg_val |= (1 << 0);
	writel(reg_val, sunxi_ccu_reg(ccu, CCU_GPADC_BGR_REG));
	return 0;
}

void sunxi_clk_init(sunxi_ccu_t *ccu) {
	printk_debug("Set SoC 1823 (H616/H313/H618) CLK Start.\n");
	set_platform_config(ccu);
	set_pll_cpux_axi(ccu);
	set_pll_periph0(ccu);
	set_ahb(ccu);
	set_apb(ccu);
	set_pll_dma(ccu);
	set_pll_mbus(ccu);
	set_modules_clock(ccu);
}

uint32_t sunxi_clk_get_peri1x_rate(sunxi_ccu_t *ccu) {
	uint32_t reg32;
	uint8_t plln, pllm, p0;

	/* PLL PERI */
	reg32 = read32(sunxi_ccu_reg(ccu, CCU_PLL_PERI0_CTRL_REG));
	if (reg32 & (1 << 31)) {
		plln = ((reg32 >> 8) & 0xff) + 1;
		pllm = (reg32 & 0x01) + 1;
		p0 = ((reg32 >> 16) & 0x03) + 1;

		return ((((24 * plln) / (pllm * p0))) * 1000 * 1000);
	}

	return 0;
}

void sunxi_clk_reset(sunxi_ccu_t *ccu) {
	uint32_t reg_val;

	/*set ahb,apb to default, use OSC24M*/
	reg_val = read32(sunxi_ccu_reg(ccu, CCU_PSI_AHB1_AHB2_CFG_REG));
	reg_val &= (~((0x3 << 24) | (0x3 << 8) | (0x3)));
	writel(reg_val, sunxi_ccu_reg(ccu, CCU_PSI_AHB1_AHB2_CFG_REG));

	reg_val = read32(sunxi_ccu_reg(ccu, CCU_APB1_CFG_GREG));
	reg_val &= (~((0x3 << 24) | (0x3 << 8) | (0x3)));
	writel(reg_val, sunxi_ccu_reg(ccu, CCU_APB1_CFG_GREG));

	/*set cpux pll to default,use OSC24M*/
	writel(0x0301, sunxi_ccu_reg(ccu, CCU_CPUX_AXI_CFG_REG));
	return;
}

void sunxi_clk_dump(sunxi_ccu_t *ccu) {
	uint32_t reg32;
	uint32_t cpu_clk_src;
	uint32_t plln, pllm;
	uint8_t p0;
	uint8_t p1;
	const char *clock_str;

	/* PLL CPU */
	reg32 = read32(sunxi_ccu_reg(ccu, CCU_CPUX_AXI_CFG_REG));
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

	reg32 = read32(sunxi_ccu_reg(ccu, CCU_PLL_CPUX_CTRL_REG));
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

	printk_debug("CLK: CPU PLL=%s FREQ=%luMHz\r\n", clock_str, ((((reg32 >> 8) & 0xff) + 1) * 24 / p1));

	/* PLL PERIx */
	reg32 = read32(sunxi_ccu_reg(ccu, CCU_PLL_PERI0_CTRL_REG));
	if (reg32 & (1 << 31)) {
		plln = ((reg32 >> 8) & 0xff) + 1;
		pllm = (reg32 & 0x01) + 1;
		p0 = ((reg32 >> 16) & 0x03) + 1;
		p1 = ((reg32 >> 20) & 0x03) + 1;

		printk_debug("CLK: PLL_PERI0 (2X)=%luMHz, (1X)=%luMHz, (1200M)=%luMHz\r\n", (24 * plln) / (pllm * p0), (24 * plln) / (pllm * p0) >> 1, (24 * plln) / (pllm * p1));
	} else {
		printk_debug("CLK: PLL_PERI0 disabled\r\n");
	}

	/* PLL PERIx */
	reg32 = read32(sunxi_ccu_reg(ccu, CCU_PLL_PERI1_CTRL_REG));
	if (reg32 & (1 << 31)) {
		plln = ((reg32 >> 8) & 0xff) + 1;
		pllm = (reg32 & 0x01) + 1;
		p0 = ((reg32 >> 16) & 0x03) + 1;
		p1 = ((reg32 >> 20) & 0x03) + 1;

		printk_debug("CLK: PLL_PERI1 (2X)=%luMHz, (1X)=%luMHz, (1200M)=%luMHz\r\n", (24 * plln) / (pllm * p0), (24 * plln) / (pllm * p0) >> 1, (24 * plln) / (pllm * p1));
	} else {
		printk_debug("CLK: PLL_PERI1 disabled\r\n");
	}

	/* PLL DDR0 */
	reg32 = read32(sunxi_ccu_reg(ccu, CCU_PLL_DDR0_CTRL_REG));
	if (reg32 & (1 << 31)) {
		plln = ((reg32 >> 8) & 0xff) + 1;

		pllm = (reg32 & 0x01) + 1;
		p1 = ((reg32 >> 1) & 0x1) + 1;
		p0 = (reg32 & 0x01) + 1;

		printk_debug("CLK: PLL_DDR0=%luMHz\r\n", (24 * plln) / (p0 * p1));

	} else {
		printk_debug("CLK: PLL_DDR0 disabled\r\n");
	}


	/* PLL DDR1 */
	reg32 = read32(sunxi_ccu_reg(ccu, CCU_PLL_DDR1_CTRL_REG));
	if (reg32 & (1 << 31)) {
		plln = ((reg32 >> 8) & 0xff) + 1;

		pllm = (reg32 & 0x01) + 1;
		p1 = ((reg32 >> 1) & 0x1) + 1;
		p0 = (reg32 & 0x01) + 1;

		printk_debug("CLK: PLL_DDR1=%luMHz\r\n", (24 * plln) / (p0 * p1));

	} else {
		printk_debug("CLK: PLL_DDR1 disabled\r\n");
	}

	/* PLL HSIC */
	reg32 = read32(sunxi_ccu_reg(ccu, CCU_PLL_HSIC_CTRL_REG));
	if (reg32 & (1 << 31)) {
		plln = ((reg32 >> 8) & 0xff) + 1;

		pllm = (reg32 & 0x01) + 1;
		p1 = ((reg32 >> 1) & 0x1) + 1;
		p0 = (reg32 & 0x01) + 1;

		printk_debug("CLK: HSIC=%luMHz\r\n", (24 * plln) / (p0 * p1));

	} else {
		printk_debug("CLK: HSIC disabled\r\n");
	}
}

DT2C_DRIVER_COMPAT("allwinner,sun50iw9-ccu");
