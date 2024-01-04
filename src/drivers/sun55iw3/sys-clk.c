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

typedef struct {
    int FactorN;
    int FactorM0;
    int FactorM1;
    int FactorP;
} core_pll_freq_fact;

static void set_bit(uint32_t cpux, u8 bit) {
    uint32_t reg_val;
    reg_val = readl(cpux);
    reg_val |= (1 << bit);
    writel(reg_val, cpux);
    printk(LOG_LEVEL_TRACE, "set_bit cpux = 0x%08x, bit = %d\n", cpux, bit);
}

static void clear_bit(uint32_t cpux, u8 bit) {
    uint32_t reg_val;
    reg_val = readl(cpux);
    reg_val &= ~(1 << bit);
    writel(reg_val, cpux);
    printk(LOG_LEVEL_TRACE, "clear_bit cpux = 0x%08x, bit = %d\n", cpux, bit);
}

static void enable_pll(uint32_t cpux, core_pll_freq_fact *CPUx, uint32_t default_val) {
    uint32_t reg_val;

    writel(default_val, cpux);
    /* disable pll gating*/
    clear_bit(cpux, PLL_CPU1_CTRL_REG_PLL_OUTPUT_GATE_OFFSET);

    /*set  PLL_CPUx ,PLL_OUTPUT= 24M*N/P/(M0*M1) */
    reg_val = readl(cpux);
    reg_val &= ~((0x3 << 20) | (0xf << 16) | (0xff << 8) | (0xf << 0));
    reg_val |= ((CPUx->FactorM0 << 20) | (CPUx->FactorP << 16) |
                (CPUx->FactorN << 8) | (CPUx->FactorM1 << 0));
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

static void set_pll_cpux_axi(void) {
    uint32_t reg_val;
    core_pll_freq_fact cpu_pll;

    writel(0x0305, CCU_PLL_CPUA_CLK_REG);
    writel(0x0305, CCU_PLL_CPUB_CLK_REG);
    sdelay(20);
    writel((0), CCU_PLL_DSU_CLK_REG);
    sdelay(20);

    printk(LOG_LEVEL_DEBUG, "periph0 has been enabled\n");

    cpu_pll.FactorM0 = 0;
    cpu_pll.FactorN = 0x2a; /*1008M*/
    cpu_pll.FactorM1 = 0;
    cpu_pll.FactorP = 0;
    enable_pll(CCU_PLL_CPU1_CTRL_REG, &cpu_pll, 0x48801400);

    cpu_pll.FactorM0 = 0;
    cpu_pll.FactorN = 0x2a; /*1008M*/
    cpu_pll.FactorM1 = 0;
    cpu_pll.FactorP = 0;
    enable_pll(CCU_PLL_CPU3_CTRL_REG, &cpu_pll, 0x48801400);

    cpu_pll.FactorM0 = 0;
    cpu_pll.FactorN = 0x27; /*936M*/
    cpu_pll.FactorM1 = 0;
    cpu_pll.FactorP = 0;
    enable_pll(CCU_PLL_CPU2_CTRL_REG, &cpu_pll, 0x48801400);

    /* PLL_CPU1 is core0~core3 clock,  select PLL_CPU1  clock src:
	 * PLL_CPU1/P ,P = 1,  */
    /*set and change cpu clk src to PLL_CPU1,  PLL_CPU1/P*/
    reg_val = readl(CCU_PLL_CPUA_CLK_REG);
    reg_val &= ~(0x07 << 24);
    reg_val |= (0x03 << 24);
    reg_val &= ~(0x03 << 16);// P = 1
    reg_val |= (0x00 << 16);
    writel(reg_val, CCU_PLL_CPUA_CLK_REG);
    sdelay(20);

    /* PLL_CPU3 is core4~core7 clock,  select PLL_CPU3  clock src:
	 * PLL_CPU3/P ,P = 1,  */
    /*set and change cpu clk src to PLL_CPU1,  PLL_CPU1/P*/
    reg_val = readl(CCU_PLL_CPUB_CLK_REG);
    reg_val &= ~(0x07 << 24);
    reg_val |= (0x03 << 24);
    reg_val &= ~(0x03 << 16);// P = 1
    reg_val |= (0x00 << 16);
    writel(reg_val, CCU_PLL_CPUB_CLK_REG);
    sdelay(20);

    /*dsu clk src to PLL_CPU2,  PLL_CPU2/P*/
    reg_val = readl(CCU_PLL_DSU_CLK_REG);
    reg_val &= ~(0x07 << 24);
    reg_val |= (0x03 << 24);
    reg_val &= ~(0x03 << 16);// P = 1
    reg_val |= (0x00 << 16);
    writel(reg_val, CCU_PLL_DSU_CLK_REG);
    sdelay(20);
}

static void set_pll_periph0(void) {
    uint32_t reg_val;

    if ((1U << 31) & readl(CCU_BASE + CCU_PLL_PERI0_CTRL_REG)) {
        /*fel has enable pll_periph0*/
        printk(LOG_LEVEL_DEBUG, "periph0 has been enabled\n");
        return;
    }

    /* set default val  24MHz * N/M1/P0 = 24 * 100 /1/2 = 1.2G*/
    writel((0x48216310), CCU_BASE + CCU_PLL_PERI0_CTRL_REG);

    /* lock enable */
    reg_val = readl(CCU_BASE + CCU_PLL_PERI0_CTRL_REG);
    reg_val |= (1 << 29);
    writel(reg_val, CCU_BASE + CCU_PLL_PERI0_CTRL_REG);

    /* lock enable */
    reg_val = readl(CCU_BASE + CCU_PLL_PERI0_CTRL_REG);
    reg_val |= (1 << 30);
    writel(reg_val, CCU_BASE + CCU_PLL_PERI0_CTRL_REG);

    /* enabe PLL: 600M(1X)  1200M(2x) 2400M(4X) */
    reg_val = readl(CCU_BASE + CCU_PLL_PERI0_CTRL_REG);
    reg_val |= (1 << 31);
    writel(reg_val, CCU_BASE + CCU_PLL_PERI0_CTRL_REG);

    while (!(readl(CCU_BASE + CCU_PLL_PERI0_CTRL_REG) & (0x1 << 28)))
        ;
    sdelay(20);
    /* lock disable */
    reg_val = readl(CCU_BASE + CCU_PLL_PERI0_CTRL_REG);
    reg_val &= (~(1 << 29));
    writel(reg_val, CCU_BASE + CCU_PLL_PERI0_CTRL_REG);
}

static void set_pll_periph1(void) {
    uint32_t reg_val;

    if ((1U << 31) & readl(CCU_BASE + CCU_PLL_PERI1_CTRL_REG)) {
        /*fel has enable pll_periph0*/
        printk(LOG_LEVEL_DEBUG, "periph0 has been enabled\n");
        return;
    }

    /* set default val  24MHz * N/M1/P0 = 24 * 100 /1/2 = 1.2G*/
    writel((0x48216310), CCU_BASE + CCU_PLL_PERI1_CTRL_REG);

    /* lock enable */
    reg_val = readl(CCU_BASE + CCU_PLL_PERI1_CTRL_REG);
    reg_val |= (1 << 29);
    writel(reg_val, CCU_BASE + CCU_PLL_PERI1_CTRL_REG);

    /* lock enable */
    reg_val = readl(CCU_BASE + CCU_PLL_PERI1_CTRL_REG);
    reg_val |= (1 << 30);
    writel(reg_val, CCU_BASE + CCU_PLL_PERI1_CTRL_REG);

    /* enabe PLL: 600M(1X)  1200M(2x) 2400M(4X) */
    reg_val = readl(CCU_BASE + CCU_PLL_PERI1_CTRL_REG);
    reg_val |= (1 << 31);
    writel(reg_val, CCU_BASE + CCU_PLL_PERI1_CTRL_REG);

    while (!(readl(CCU_BASE + CCU_PLL_PERI1_CTRL_REG) & (0x1 << 28)))
        ;
    sdelay(20);
    /* lock disable */
    reg_val = readl(CCU_BASE + CCU_PLL_PERI1_CTRL_REG);
    reg_val &= (~(1 << 29));
    writel(reg_val, CCU_BASE + CCU_PLL_PERI1_CTRL_REG);
}

static void set_ahb(void) {
    /* PLL6:AHB1:APB1 = 600M:200M  AHB = src clk/M */
    writel((2 << 0), CCU_BASE + CCU_AHB0_CFG_REG);
    writel((0x03 << 24) | readl(CCU_BASE + CCU_AHB0_CFG_REG), CCU_BASE + CCU_AHB0_CFG_REG);
    sdelay(1);
}

static void set_apb(void) {
    /*PLL6:APB0 = 600M:100M  APB = src clk/M */
    writel((5 << 0), CCU_BASE + CCU_APB0_CFG_REG);
    writel((0x03 << 24) | readl(CCU_BASE + CCU_APB0_CFG_REG), CCU_BASE + CCU_APB0_CFG_REG);
    sdelay(1);
}

static void set_pll_dma(void) {
    /*dma reset*/
    writel(readl(CCU_BASE + CCU_DMA_BGR_REG) | (1 << 16), CCU_BASE + CCU_DMA_BGR_REG);
    sdelay(20);
    /*gating clock for dma pass*/
    writel(readl(CCU_BASE + CCU_DMA_BGR_REG) | (1 << 0), CCU_BASE + CCU_DMA_BGR_REG);
}

static void set_pll_mbus(void) {
    uint32_t reg_val = 0;

    /*reset mbus domain*/
    reg_val |= (0x1 << 30);
    writel(reg_val, CCU_BASE + CCU_MBUS_CFG_REG);
    sdelay(1);

    /* set MBUS div   m = 1*/
    /* set MBUS clock source to pllddr, mbus=pllddr/(m+1) = 933*2/4 = 466M */
    reg_val |= (0x0 << 24);
    reg_val |= 0x3;
    /* open MBUS clock */
    reg_val |= (0X01 << 31);
    /* set mbus upd bit*/
    reg_val |= (0x1 << 27);
    writel(reg_val, CCU_BASE + CCU_MBUS_CFG_REG);
    sdelay(1);
}

static void set_circuits_analog(void) {
    /* calibration circuits analog enable */
    setbits_le32(VDD_SYS_PWROFF_GATING_REG, 0x01 << VDD_ADDA_OFF_GATING);
    sdelay(1);

    setbits_le32(RES_CAL_CTRL_REG, 0x01 << CAL_ANA_EN);
    sdelay(1);

    clrbits_le32(RES_CAL_CTRL_REG, 0x01 << CAL_EN);
    sdelay(1);

    setbits_le32(RES_CAL_CTRL_REG, 0x01 << CAL_EN);
    sdelay(1);
}

static inline void set_iommu_auto_gating(void) {
    /*gating clock for iommu*/
    writel(0x01, CCU_BASE + CCU_IOMMU_BGR_REG);
    /*enable auto gating*/
    writel(0x01, IOMMU_AUTO_GATING_REG);
}

static void set_platform_config(void) {
    set_circuits_analog();
    set_iommu_auto_gating();
}

void sunxi_clk_init(void) {
    printk(LOG_LEVEL_DEBUG, "Set SoC 1890 (A523/A527/MR527/T527) CLK Start.\n");
    set_platform_config();
    set_pll_cpux_axi();
    set_pll_periph0();
    set_pll_periph1();
    set_ahb();
    set_apb();
    set_pll_dma();
    set_pll_mbus();
    printk(LOG_LEVEL_DEBUG, "Set pll done\n");
    return;
}

void sunxi_clk_reset(void) {
    uint32_t reg_val;
    /*set ahb,apb to default, use OSC24M*/
    reg_val = readl(CCU_BASE + CCU_AHB0_CFG_REG);
    reg_val &= (~(0x3 << 24));
    writel(reg_val, CCU_BASE + CCU_AHB0_CFG_REG);

    reg_val = readl(CCU_BASE + CCU_APB0_CFG_REG);
    reg_val &= (~(0x3 << 24));
    writel(reg_val, CCU_BASE + CCU_APB0_CFG_REG);

    /*set cpux pll to default,use OSC24M*/
    writel(0x0305, CCU_BASE + CCU_PLL_CPUA_CLK_REG);
    return;
}

uint32_t sunxi_clk_get_peri1x_rate() {
    uint32_t reg32;
    uint8_t plln, pllm, p0;

    /* PLL PERI */
    reg32 = read32(CCU_BASE + CCU_PLL_PERI0_CTRL_REG);
    if (reg32 & (1 << 31)) {
        plln = ((reg32 >> 8) & 0xff) + 1;
        pllm = (reg32 & 0x01) + 1;
        p0 = ((reg32 >> 16) & 0x03) + 1;

        return ((((24 * plln) / (pllm * p0))) * 1000 * 1000);
    }

    return 0;
}

static void sunxi_cpux_clk_dump(uint8_t cpuid, uint32_t cpu_reg) {
    uint32_t reg32;
    uint8_t p0;
    uint8_t p1;

    reg32 = read32(cpu_reg);
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

    printk(LOG_LEVEL_DEBUG, "CLK: CPU%d FREQ=%luMHz\r\n", cpuid, ((((reg32 >> 8) & 0xff) + 1) * 24 / p1));
}

void sunxi_clk_dump() {
    uint32_t reg32;
    uint32_t cpu_clk_src;
    uint32_t plln, pllm;
    uint8_t p0;
    uint8_t p1;

    sunxi_cpux_clk_dump(0, CCU_PLL_CPU0_CTRL_REG);
    sunxi_cpux_clk_dump(1, CCU_PLL_CPU1_CTRL_REG);
    sunxi_cpux_clk_dump(2, CCU_PLL_CPU2_CTRL_REG);
    sunxi_cpux_clk_dump(3, CCU_PLL_CPU3_CTRL_REG);

    /* PLL PERIx */
    reg32 = read32(CCU_BASE + CCU_PLL_PERI0_CTRL_REG);
    if (reg32 & (1 << 31)) {
        plln = ((reg32 >> 8) & 0xff) + 1;
        pllm = (reg32 & 0x01) + 1;
        p0 = ((reg32 >> 16) & 0x03) + 1;
        p1 = ((reg32 >> 20) & 0x03) + 1;

        printk(LOG_LEVEL_DEBUG, "CLK: PLL_PERI0 (2X)=%luMHz, (1X)=%luMHz, (800M)=%luMHz\r\n", (24 * plln) / (pllm * p0),
               (24 * plln) / (pllm * p0) >> 1, (24 * plln) / (pllm * p1));
    } else {
        printk(LOG_LEVEL_DEBUG, "CLK: PLL_PERI0 disabled\r\n");
    }

    /* PLL PERIx */
    reg32 = read32(CCU_BASE + CCU_PLL_PERI1_CTRL_REG);
    if (reg32 & (1 << 31)) {
        plln = ((reg32 >> 8) & 0xff) + 1;
        pllm = (reg32 & 0x01) + 1;
        p0 = ((reg32 >> 16) & 0x03) + 1;
        p1 = ((reg32 >> 20) & 0x03) + 1;

        printk(LOG_LEVEL_DEBUG, "CLK: PLL_PERI1 (2X)=%luMHz, (1X)=%luMHz, (800M)=%luMHz\r\n", (24 * plln) / (pllm * p0),
               (24 * plln) / (pllm * p0) >> 1, (24 * plln) / (pllm * p1));
    } else {
        printk(LOG_LEVEL_DEBUG, "CLK: PLL_PERI1 disabled\r\n");
    }

    /* PLL DDR0 */
    reg32 = read32(CCU_BASE + CCU_PLL_DDR0_CTRL_REG);
    if (reg32 & (1 << 31)) {
        plln = ((reg32 >> 8) & 0xff) + 1;

        pllm = (reg32 & 0x01) + 1;
        p1 = ((reg32 >> 1) & 0x1) + 1;
        p0 = (reg32 & 0x01) + 1;

        printk(LOG_LEVEL_DEBUG, "CLK: PLL_DDR0=%luMHz\r\n", (24 * plln) / (p0 * p1));

    } else {
        printk(LOG_LEVEL_DEBUG, "CLK: PLL_DDR0 disabled\r\n");
    }


    /* PLL DDR1 */
    reg32 = read32(CCU_BASE + CCU_PLL_DDR1_CTRL_REG);
    if (reg32 & (1 << 31)) {
        plln = ((reg32 >> 8) & 0xff) + 1;

        pllm = (reg32 & 0x01) + 1;
        p1 = ((reg32 >> 1) & 0x1) + 1;
        p0 = (reg32 & 0x01) + 1;

        printk(LOG_LEVEL_DEBUG, "CLK: PLL_DDR1=%luMHz\r\n", (24 * plln) / (p0 * p1));

    } else {
        printk(LOG_LEVEL_DEBUG, "CLK: PLL_DDR1 disabled\r\n");
    }

    /* PLL HSIC */
    reg32 = read32(CCU_BASE + CCU_PLL_HSIC_CTRL_REG);
    if (reg32 & (1 << 31)) {
        plln = ((reg32 >> 8) & 0xff) + 1;

        pllm = (reg32 & 0x01) + 1;
        p1 = ((reg32 >> 1) & 0x1) + 1;
        p0 = (reg32 & 0x01) + 1;

        printk(LOG_LEVEL_DEBUG, "CLK: HSIC=%luMHz\r\n", (24 * plln) / (p0 * p1));

    } else {
        printk(LOG_LEVEL_DEBUG, "CLK: HSIC disabled\r\n");
    }
}
