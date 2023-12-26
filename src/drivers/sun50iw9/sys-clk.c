/* SPDX-License-Identifier: Apache-2.0 */

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <timer.h>

#include <log.h>

#include <sys-clk.h>

void set_pll_cpux_axi(void) {
    uint32_t reg_val;
    /* select CPUX  clock src: OSC24M,AXI divide ratio is 2, system apb clk ratio is 4 */
    writel((0 << 24) | (3 << 8) | (1 << 0), CCU_CPUX_AXI_CFG_REG);
    sdelay(1);

    /* disable pll*/
    reg_val = readl(CCU_PLL_CPUX_CTRL_REG);
    reg_val &= ~(1 << 31);
    writel(reg_val, CCU_PLL_CPUX_CTRL_REG);

    /* set default val: clk is 1008M  ,PLL_OUTPUT= 24M*N/( M*P)*/
    reg_val = readl(CCU_PLL_CPUX_CTRL_REG);
    reg_val &= ~((0x3 << 16) | (0xff << 8) | (0x3 << 0));
    reg_val |= (41 << 8);
    writel(reg_val, CCU_PLL_CPUX_CTRL_REG);

    /* lock enable */
    reg_val = readl(CCU_PLL_CPUX_CTRL_REG);
    reg_val |= (1 << 29);
    writel(reg_val, CCU_PLL_CPUX_CTRL_REG);

    /* enable pll */
    reg_val = readl(CCU_PLL_CPUX_CTRL_REG);
    reg_val |= (1 << 31);
    writel(reg_val, CCU_PLL_CPUX_CTRL_REG);

    /* wait PLL_CPUX stable */
    while (!(readl(CCU_PLL_CPUX_CTRL_REG) & (0x1 << 28)))
        ;
    sdelay(20);

    /* lock disable */
    reg_val = readl(CCU_PLL_CPUX_CTRL_REG);
    reg_val &= ~(1 << 29);
    writel(reg_val, CCU_PLL_CPUX_CTRL_REG);

    //set and change cpu clk src to PLL_CPUX,  PLL_CPUX:AXI0 = 408M:136M
    reg_val = readl(CCU_CPUX_AXI_CFG_REG);
    reg_val &= ~(0x03 << 24);
    reg_val |= (0x03 << 24);
    writel(reg_val, CCU_CPUX_AXI_CFG_REG);

    sdelay(1);
}

void set_pll_periph0(void) {
    uint32_t reg_val;

    if ((1U << 31) & readl(CCU_PLL_PERI0_CTRL_REG)) {
        //fel has enable pll_periph0
        printk(LOG_LEVEL_DEBUG, "periph0 has been enabled\n");
        return;
    }

    /* set default val*/
    writel(0x63 << 8, CCU_PLL_PERI0_CTRL_REG);

    /* lock enable */
    reg_val = readl(CCU_PLL_PERI0_CTRL_REG);
    reg_val |= (1 << 29);
    writel(reg_val, CCU_PLL_PERI0_CTRL_REG);

    /* enabe PLL: 600M(1X)  1200M(2x) 2400M(4X) */
    reg_val = readl(CCU_PLL_PERI0_CTRL_REG);
    reg_val |= (1 << 31);
    writel(reg_val, CCU_PLL_PERI0_CTRL_REG);

    /* lock disable */
    reg_val = readl(CCU_PLL_PERI0_CTRL_REG);
    reg_val &= (~(1 << 29));
    writel(reg_val, CCU_PLL_PERI0_CTRL_REG);
}

void set_ahb(void) {
    /* PLL6:AHB1:APB1 = 600M:200M:100M */
    writel((2 << 0) | (0 << 8), CCU_PSI_AHB1_AHB2_CFG_REG);
    writel((0x03 << 24) | readl(CCU_PSI_AHB1_AHB2_CFG_REG), CCU_PSI_AHB1_AHB2_CFG_REG);
    sdelay(1);
    /*PLL6:AHB3 = 600M:200M*/
    writel((2 << 0) | (0 << 8), CCU_AHB3_CFG_GREG);
    writel((0x03 << 24) | readl(CCU_AHB3_CFG_GREG), CCU_AHB3_CFG_GREG);
}

void set_apb(void) {
    /*PLL6:APB1 = 600M:100M */
    writel((2 << 0) | (1 << 8), CCU_APB1_CFG_GREG);
    writel((0x03 << 24) | readl(CCU_APB1_CFG_GREG), CCU_APB1_CFG_GREG);
    sdelay(1);
}

void set_pll_dma(void) {
    /*dma reset*/
    writel(readl(CCU_DMA_BGR_REG) | (1 << 16), CCU_DMA_BGR_REG);
    sdelay(20);
    /*gating clock for dma pass*/
    writel(readl(CCU_DMA_BGR_REG) | (1 << 0), CCU_DMA_BGR_REG);
}

void set_pll_mbus(void) {
    uint32_t reg_val;

    /*reset mbus domain*/
    reg_val = 1 << 30;
    writel(1 << 30, CCU_MBUS_CFG_REG);
    sdelay(1);

    /* set MBUS div */
    reg_val = readl(CCU_MBUS_CFG_REG);
    reg_val |= (2 << 0);
    writel(reg_val, CCU_MBUS_CFG_REG);
    sdelay(1);

    /* set MBUS clock source to pll6(2x), mbus=pll6/(m+1) = 400M*/
    reg_val = readl(CCU_MBUS_CFG_REG);
    reg_val |= (1 << 24);
    writel(reg_val, CCU_MBUS_CFG_REG);
    sdelay(1);

    /* open MBUS clock */
    reg_val = readl(CCU_MBUS_CFG_REG);
    reg_val |= (0X01 << 31);
    writel(reg_val, CCU_MBUS_CFG_REG);
    sdelay(1);
}

void set_circuits_analog(void) {
    /* calibration circuits analog enable */
    uint32_t reg_val;

    reg_val = readl(ANA_PWR_RST_REG);
    reg_val |= (0x01 << VDD_ADDA_OFF_GATING);
    writel(reg_val, ANA_PWR_RST_REG);
    sdelay(1);

    reg_val = readl(RES_CAL_CTRL_REG);
    reg_val |= (0x01 << CAL_ANA_EN);
    writel(reg_val, RES_CAL_CTRL_REG);
    sdelay(1);

    reg_val = readl(RES_CAL_CTRL_REG);
    reg_val &= ~(0x01 << CAL_EN);
    writel(reg_val, RES_CAL_CTRL_REG);
    sdelay(1);

    reg_val = readl(RES_CAL_CTRL_REG);
    reg_val |= (0x01 << CAL_EN);
    writel(reg_val, RES_CAL_CTRL_REG);
    sdelay(1);
}

static inline void set_iommu_auto_gating(void) {
    /*gating clock for iommu*/
    writel(0x01, CCU_IOMMU_BGR_REG);
    /*enable auto gating*/
    writel(0x01, IOMMU_AUTO_GATING_REG);
}

void set_platform_config(void) {
    /* 
     * At present, the audio codec finds a problem. VRA1 does not accelerate the power-on,
	 * which will affect the stability of the bais circuit and affect the boot speed.
	 * Therefore, the river vddon needs to be set to 1.
	*/
    set_circuits_analog();
    set_iommu_auto_gating();
}

void set_modules_clock(void) {
    uint32_t reg_val;
    uint32_t reg_addr;

    /*enable peri1 clk*/
    reg_addr = CCU_BASE + 0x28;
    reg_val = readl(reg_addr);
    reg_val |= (1 << 31);
    writel(reg_val, reg_addr);
    sdelay(10);

    /*enable gpu clk*/
    reg_addr = CCU_BASE + 0x30;
    reg_val = readl(reg_addr);
    reg_val |= (1 << 31);
    writel(reg_val, reg_addr);
    sdelay(10);

    /*enable video0 clk*/
    reg_addr = CCU_BASE + 0x40;
    reg_val = readl(reg_addr);
    reg_val |= (1 << 31);
    writel(reg_val, reg_addr);
    sdelay(10);

    /*enable video1 clk*/
    reg_addr = CCU_BASE + 0x48;
    reg_val = readl(reg_addr);
    reg_val |= (1 << 31);
    writel(reg_val, reg_addr);
    sdelay(10);

    /*enable video2 clk*/
    reg_addr = CCU_BASE + 0x50;
    reg_val = readl(reg_addr);
    reg_val |= (1 << 31);
    writel(reg_val, reg_addr);
    sdelay(10);

    /*enable ve clk*/
    reg_addr = CCU_BASE + 0x58;
    reg_val = readl(reg_addr);
    reg_val |= (1 << 31);
    writel(reg_val, reg_addr);
    sdelay(10);

    /*enable de clk*/
    reg_addr = CCU_BASE + 0x60;
    reg_val = readl(reg_addr);
    reg_val |= (1 << 31);
    writel(reg_val, reg_addr);
    sdelay(10);

    /*enable csi clk*/
    reg_addr = CCU_BASE + 0xE0;
    reg_val = readl(reg_addr);
    reg_val |= (1 << 31);
    writel(reg_val, reg_addr);
    sdelay(10);

    /*enable audio clk*/
    reg_addr = CCU_BASE + 0x78;
    reg_val = readl(reg_addr);
    reg_val |= (1 << 31);
    writel(reg_val, reg_addr);
    sdelay(10);
}

int sunxi_clock_init_gpadc(void) {
    uint32_t reg_val = 0;
    /* reset */
    reg_val = readl(CCU_GPADC_BGR_REG);
    reg_val &= ~(1 << 16);
    writel(reg_val, CCU_GPADC_BGR_REG);

    sdelay(2);

    reg_val |= (1 << 16);
    writel(reg_val, CCU_GPADC_BGR_REG);
    /* enable KEYADC gating */
    reg_val = readl(CCU_GPADC_BGR_REG);
    reg_val |= (1 << 0);
    writel(reg_val, CCU_GPADC_BGR_REG);
    return 0;
}

void sunxi_clk_init(void) {
    set_platform_config();
    set_pll_cpux_axi();
    set_pll_periph0();
    set_ahb();
    set_apb();
    set_pll_dma();
    set_pll_mbus();
    set_modules_clock();
}

void sunxi_clk_reset(void) {
    uint32_t reg_val;

    /*set ahb,apb to default, use OSC24M*/
    reg_val = readl(CCU_PSI_AHB1_AHB2_CFG_REG);
    reg_val &= (~((0x3 << 24) | (0x3 << 8) | (0x3)));
    writel(reg_val, CCU_PSI_AHB1_AHB2_CFG_REG);

    reg_val = readl(CCU_APB1_CFG_GREG);
    reg_val &= (~((0x3 << 24) | (0x3 << 8) | (0x3)));
    writel(reg_val, CCU_APB1_CFG_GREG);

    /*set cpux pll to default,use OSC24M*/
    writel(0x0301, CCU_CPUX_AXI_CFG_REG);
    return;
}

void sunxi_clk_dump() {
    uint32_t reg32;
    uint32_t cpu_clk_src;
    uint32_t plln, pllm;
    uint8_t p0;
    uint8_t p1;
    const char *clock_str;

    /* PLL CPU */
    reg32 = read32(CCU_BASE + CCU_CPUX_AXI_CFG_REG);
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

    reg32 = read32(CCU_BASE + CCU_PLL_CPUX_CTRL_REG);
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

    printk(LOG_LEVEL_DEBUG, "CLK: CPU PLL=%s FREQ=%" PRIu32 "MHz\r\n", clock_str, ((((reg32 >> 8) & 0xff) + 1) * 24 / p1));

    /* PLL PERIx */
    reg32 = read32(CCU_BASE + CCU_PLL_PERI0_CTRL_REG);
    if (reg32 & (1 << 31)) {
        plln = ((reg32 >> 8) & 0xff) + 1;
        pllm = (reg32 & 0x01) + 1;
        p0 = ((reg32 >> 16) & 0x03) + 1;
        p1 = ((reg32 >> 20) & 0x03) + 1;

        printk(LOG_LEVEL_DEBUG, "CLK: PLL_peri (2X)=%" PRIu32 "MHz, (1X)=%" PRIu32 "MHz, (800M)=%" PRIu32 "MHz\r\n", (24 * plln) / (pllm * p0),
               (24 * plln) / (pllm * p0) >> 1, (24 * plln) / (pllm * p1));
    } else {
        printk(LOG_LEVEL_DEBUG, "CLK: PLL_peri disabled\r\n");
    }

    /* PLL DDR */
    reg32 = read32(CCU_BASE + CCU_PLL_DDR0_CTRL_REG);
    if (reg32 & (1 << 31)) {
        plln = ((reg32 >> 8) & 0xff) + 1;

        pllm = (reg32 & 0x01) + 1;
        p1 = ((reg32 >> 1) & 0x1) + 1;
        p0 = (reg32 & 0x01) + 1;

        printk(LOG_LEVEL_DEBUG, "CLK: PLL_ddr=%" PRIu32 "MHz\r\n", (24 * plln) / (p0 * p1));

    } else {
        printk(LOG_LEVEL_DEBUG, "CLK: PLL_ddr disabled\r\n");
    }
}