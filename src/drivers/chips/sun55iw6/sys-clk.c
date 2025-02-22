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

#define CPU_UPDATE_OFFSET (26)
#define CPU_LOCK_OFFSET (28)
#define CPU_LOCK_ENABLE_OFFSET (29)

static void enable_pll(uint32_t reg_addr, uint32_t n_factor) {
    uint32_t reg_val = 0;
    reg_val = readl(reg_addr);
    //--cfg pll
    //set n=0x28,m0=m1=1,p=1
    //24M*n/p/(m0 * m1)
    reg_val &= ~((0xff << 8) | (0x7 << 16) | (0x3 << 20) | (0xf << 0));
    reg_val |= (n_factor << 8) | (0x0 << 0);
    //lock enable
    reg_val &= (~(0x1 << CPU_LOCK_ENABLE_OFFSET));
    writel(reg_val, reg_addr);
    reg_val |= (0x1 << CPU_LOCK_ENABLE_OFFSET);
    writel(reg_val, reg_addr);

    //update_bit
    reg_val = readl(reg_addr);
    reg_val |= (0x1 << CPU_UPDATE_OFFSET);
    writel(reg_val, reg_addr);
    do {
        reg_val = readl(reg_addr);
        reg_val = reg_val & (0x1 << CPU_UPDATE_OFFSET);
    } while (reg_val);

    udelay(26);
    do {
        reg_val = readl(reg_addr);
        reg_val = reg_val & (0x1 << CPU_LOCK_OFFSET);
    } while (!reg_val);
}

static void set_pll_cpux_axi(void) {
    uint32_t reg_val;

    enable_pll(CCU_REG_PLL_C0_CPUX, 0x2a);
    enable_pll(CCU_REG_PLL_C0_DSU, 0x16);

    /* set cpu_axi_div factor M */
    reg_val = readl(CCU_REG_DSU_CLK);
    reg_val &= ~(0x3);
    writel(reg_val, CCU_REG_DSU_CLK);
}

static void set_apb(void) {
    uint32_t reg_value = 0;
    reg_value = readl(CCU_APB1_CFG_GREG);
    reg_value &= ~APB1_CLK_REG_CLK_SRC_SEL_CLEAR_MASK;
    reg_value |= (APB1_CLK_REG_CLK_SRC_SEL_HOSC
                  << APB1_CLK_REG_CLK_SRC_SEL_OFFSET);
    writel(reg_value, CCU_APB1_CFG_GREG);
    udelay(10);

    reg_value = readl(CCU_APB1_CFG_GREG);
    reg_value &= ~APB1_CLK_REG_FACTOR_M_CLEAR_MASK;
    writel(reg_value, CCU_APB1_CFG_GREG);
    udelay(10);
}

static void set_pll_nsi(void) {
    uint32_t reg_val;
    uint32_t time_cnt = 0;

    /* disable nsi_clk_gating */
    reg_val = readl(CCU_NSI_CLK_GREG);
    reg_val &= ~(0x1U << NSI_CLK_REG_NSI_CLK_GATING_OFFSET);
    reg_val &= ~(NSI_CLK_REG_NSI_DIV1_CLEAR_MASK);
    reg_val |= (0x5U << NSI_CLK_REG_NSI_DIV1_OFFSET);
    reg_val |= 1 << NSI_CLK_REG_NSI_UPD_OFFSET;//update
    writel(reg_val, CCU_NSI_CLK_GREG);
    do {
        reg_val = readl(CCU_NSI_CLK_GREG);
        reg_val = reg_val & (0x1U << NSI_CLK_REG_NSI_UPD_OFFSET);
        udelay(1);

        if (reg_val && (++time_cnt >= 100000)) {
            printk_debug("nsi clk gating update failed!\n");
            break;
        }
    } while (reg_val);

    time_cnt = 0;
    reg_val = readl(CCU_NSI_CLK_GREG);
    /* set NSI clock source to ddr pll */
    reg_val &= ~(NSI_CLK_REG_NSI_CLK_SEL_CLEAR_MASK);
    reg_val |= (NSI_CLK_REG_NSI_CLK_SEL_DDRPLL
                << NSI_CLK_REG_NSI_CLK_SEL_OFFSET);
    /* open NSI clock */
    reg_val |= (0X01 << NSI_CLK_REG_NSI_CLK_GATING_OFFSET);
    /* update_bit */
    reg_val |= (0x1U << NSI_CLK_REG_NSI_UPD_OFFSET);
    writel(reg_val, CCU_NSI_CLK_GREG);
    do {
        reg_val = readl(CCU_NSI_CLK_GREG);
        reg_val = reg_val & (0x1U << NSI_CLK_REG_NSI_UPD_OFFSET);
        udelay(1);

        if (reg_val && (++time_cnt >= 100000)) {
            printk_debug("nsi clk update failed!\n");
            break;
        }
    } while (reg_val);
}

static void set_pll_mbus(void) {
    uint32_t reg_val = 0x0;
    uint32_t time_cnt = 0;

    //--disable mbus_clk_gating
    reg_val = readl(CCU_MBUS_CFG_REG);
    reg_val &= ~(0x1U << MBUS_CLK_REG_MBUS_CLK_GATING_OFFSET);
    reg_val &= ~(MBUS_CLK_REG_MBUS_DIV1_CLEAR_MASK);
    reg_val |= (0x5U << MBUS_CLK_REG_MBUS_DIV1_OFFSET);
    reg_val |= 1 << MBUS_CLK_REG_MBUS_UPD_OFFSET;//update
    writel(reg_val, CCU_MBUS_CFG_REG);
    do {
        reg_val = readl(CCU_MBUS_CFG_REG);
        reg_val = reg_val & (0x1U << MBUS_CLK_REG_MBUS_UPD_OFFSET);
        udelay(1);

        if (reg_val && (++time_cnt >= 100000)) {
            printk_debug("mbus clk gating update failed!\n");
            break;
        }
    } while (reg_val);

    time_cnt = 0;

    reg_val = readl(CCU_MBUS_CFG_REG);
    /* set MBUS clock source to pll ddr*/
    reg_val &= ~(MBUS_CLK_REG_MBUS_CLK_SEL_CLEAR_MASK);
    reg_val |= (MBUS_CLK_REG_MBUS_CLK_SEL_DDRPLL
                << MBUS_CLK_REG_MBUS_CLK_SEL_OFFSET);
    /* open MBUS clock */
    reg_val |= (0X01 << MBUS_CLK_REG_MBUS_CLK_GATING_OFFSET);
    //update_bit
    reg_val |= (0x1U << MBUS_CLK_REG_MBUS_UPD_OFFSET);
    writel(reg_val, CCU_MBUS_CFG_REG);
    do {
        reg_val = readl(CCU_MBUS_CFG_REG);
        reg_val = reg_val & (0x1U << MBUS_CLK_REG_MBUS_UPD_OFFSET);
        udelay(1);

        if (reg_val && (++time_cnt >= 100000)) {
            printk_debug("mbus clk update failed!\n");
            break;
        }
    } while (reg_val);
}

void sunxi_clk_init(void) {
    printk_debug("set pll start\n");
    set_pll_cpux_axi();
    set_apb();
    set_pll_nsi();
    set_pll_mbus();
    printk_debug("set pll end\n");
}

void sunxi_clk_dump() {
}