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

#define PLL_REG_CONF(x) \
    { x, BIT(x##_PLL_FREF_SEL_OFFSET) }

#define CPU_PLL_FACTOR_N_24M(x) (((x) + (24) - 1) / (24))
#define CPU_PLL_FACTOR_N_26M(x) (((x) + (26) - 1) / (26))

const struct pll_reg_config {
    uint32_t reg_addr;
    uint32_t ref_sel_offset;
} pll_ctrl_regs[] = {
        PLL_REG_CONF(PLL_DDR_CTRL_REG),
        PLL_REG_CONF(PLL_PERI0_CTRL_REG),
        PLL_REG_CONF(PLL_PERI1_CTRL_REG),
        PLL_REG_CONF(PLL_GPU0_CTRL_REG),
        PLL_REG_CONF(PLL_VIDEO0_CTRL_REG),
        PLL_REG_CONF(PLL_VIDEO1_CTRL_REG),
        PLL_REG_CONF(PLL_VIDEO2_CTRL_REG),
        PLL_REG_CONF(PLL_VE0_CTRL_REG),
        PLL_REG_CONF(PLL_VE1_CTRL_REG),
        PLL_REG_CONF(PLL_AUDIO0_CTRL_REG),
        PLL_REG_CONF(PLL_AUDIO1_CTRL_REG),
        PLL_REG_CONF(PLL_NPU_CTRL_REG),
        PLL_REG_CONF(PLL_DE_CTRL_REG),
};

static inline void set_pll_parent(void) {
    for (size_t i = 0; i < sizeof(pll_ctrl_regs) / sizeof(pll_ctrl_regs[0]); i++) {
        setbits_le32(SUNXI_CCU_BASE + pll_ctrl_regs[i].reg_addr, pll_ctrl_regs[i].ref_sel_offset);
    }
}

static inline void enable_pll(uint32_t addr, uint32_t m0, uint32_t n, uint32_t m1, uint32_t p) {
    uint32_t reg_val;

    setbits_le32(addr, BIT(PLL_CPU_CTRL_REG_PLL_LDO_EN_OFFSET));
    setbits_le32(addr, BIT(PLL_CPU_CTRL_REG_PLL_OUTPUT_GATE_OFFSET));

    reg_val = read32(addr);
    reg_val &= ~((0x3 << 20) | (0xf << 16) | (0xff << 8) | (0xf << 0));
    reg_val |= ((m0 << 20) | (p << 16) | (n << 8) | (m1 << 0));
    write32(addr, reg_val);

    /* delay for pll */
    udelay(20);

    /* pll enable */
    setbits_le32(addr, BIT(PLL_CPU_CTRL_REG_PLL_EN_OFFSET));

    /* lock enable */
    setbits_le32(addr, BIT(PLL_CPU_CTRL_REG_LOCK_ENABLE_OFFSET));

    /* enable update bit */
    setbits_le32(addr, BIT(26));

    /* wait PLL_CPUX lockbit */
    while (read32(addr) & BIT(26))
        ;


    for (int i = 0; i < 3; i++) {
        while (!(read32(addr) & BIT(28)))
            ;
    }
    udelay(20);
}

static inline void set_pll(uint32_t addr, uint32_t m0, uint32_t n, uint32_t m1, uint32_t p) {
    uint32_t reg_val;

    /* set pll source to 24M */
    clrbits_le32(addr + 0x1c, 0x3 << 24);
    /* clear pll lock */
    clrbits_le32(addr, BIT(PLL_CPU_CTRL_REG_PLL_OUTPUT_GATE_OFFSET) | BIT(PLL_CPU_CTRL_REG_LOCK_ENABLE_OFFSET));
    reg_val = read32(addr);
    reg_val &= ~((0x3 << 20) | (0xf << 16) | (0xff << 8) | (0xf << 0));
    reg_val |= ((m0 << 20) | (p << 16) | (n << 8) | (m1 << 0));
    write32(addr, reg_val);

    /* delay for pll */
    udelay(20);

    printk_trace("CLK: PLL CPU 0x%08x: 0x%08x, m0 = %d, n = %d, m1 = %d, p = %d\n",
                 addr, read32(addr), m0, n, m1, p);

    /* lock enable */
    setbits_le32(addr, BIT(PLL_CPU_CTRL_REG_LOCK_ENABLE_OFFSET));

    /* enable update bit */
    setbits_le32(addr, BIT(26));

    /* wait PLL_CPUX lockbit */
    while (read32(addr) & BIT(26))
        ;

    for (int i = 0; i < 3; i++) {
        while (!(read32(addr) & BIT(28)))
            ;
    }
    udelay(20);

    /* enable pll output */
    setbits_le32(addr, BIT(PLL_CPU_CTRL_REG_PLL_OUTPUT_GATE_OFFSET));
    printk_trace("CLK: PLL CPU 0x%08x: 0x%08x\n", addr, read32(addr));
    udelay(20);
}

static inline void set_pll_cpux_axi(void) {
    if (sunxi_clk_get_hosc_type() == 24) {
        /* Set A76 Core 1.008GHz, A55 Core 1.008GHz, DSU 744MHz */
        enable_pll(CCU_PLL_CPU_L_CTRL_REG, 0x0, CPU_PLL_FACTOR_N_24M(480), 0x0, 0x0);
        set_pll(CCU_PLL_CPU_L_CTRL_REG, 0x0, CPU_PLL_FACTOR_N_24M(1008), 0x0, 0x0);
        enable_pll(CCU_PLL_CPU_B_CTRL_REG, 0x0, CPU_PLL_FACTOR_N_24M(480), 0x0, 0x0);
        set_pll(CCU_PLL_CPU_B_CTRL_REG, 0x0, CPU_PLL_FACTOR_N_24M(1008), 0x0, 0x0);
        enable_pll(CCU_PLL_CPU_DSU_CTRL_REG, 0x0, CPU_PLL_FACTOR_N_24M(480), 0x0, 0x0);
        set_pll(CCU_PLL_CPU_DSU_CTRL_REG, 0x0, CPU_PLL_FACTOR_N_24M(744), 0x0, 0x0);
    } else {
        /* Set A76 Core 1.014GHz, A55 Core 1.014GHz, DSU 780MHz */
        enable_pll(CCU_PLL_CPU_L_CTRL_REG, 0x0, CPU_PLL_FACTOR_N_26M(480), 0x0, 0x0);
        set_pll(CCU_PLL_CPU_L_CTRL_REG, 0x0, CPU_PLL_FACTOR_N_26M(1014), 0x0, 0x0);
        enable_pll(CCU_PLL_CPU_B_CTRL_REG, 0x0, CPU_PLL_FACTOR_N_26M(480), 0x0, 0x0);
        set_pll(CCU_PLL_CPU_B_CTRL_REG, 0x0, CPU_PLL_FACTOR_N_26M(1014), 0x0, 0x0);
        enable_pll(CCU_PLL_CPU_DSU_CTRL_REG, 0x0, CPU_PLL_FACTOR_N_26M(480), 0x0, 0x0);
        set_pll(CCU_PLL_CPU_DSU_CTRL_REG, 0x0, CPU_PLL_FACTOR_N_26M(780), 0x0, 0x0);
    }
    printk_debug("CLK: PLL CPU A76: 0x%08x\n", read32(CCU_PLL_CPU_L_CTRL_REG));
    printk_debug("CLK: PLL CPU A55: 0x%08x\n", read32(CCU_PLL_CPU_B_CTRL_REG));
    printk_debug("CLK: PLL CPU DSU: 0x%08x\n", read32(CCU_PLL_CPU_DSU_CTRL_REG));
    udelay(20);
    clrsetbits_le32(CCU_PLL_CPU_L_CLK_REG, (0x07 << 24) | (0x03 << 16), (0x03 << 24) | (0x00 << 16));
    udelay(20);
    clrsetbits_le32(CCU_PLL_CPU_B_CLK_REG, (0x07 << 24) | (0x03 << 16), (0x03 << 24) | (0x00 << 16));
    udelay(20);
    clrsetbits_le32(CCU_PLL_DSU_CLK_REG, (0x07 << 24) | (0x03 << 16), (0x03 << 24) | (0x00 << 16));
}

static inline void set_apb1(void) {
    clrsetbits_le32(SUNXI_CCU_BASE + APB1_CLK_REG,
                    APB1_CLK_REG_CLK_SRC_SEL_CLEAR_MASK | APB1_CLK_REG_FACTOR_M_CLEAR_MASK,
                    APB1_CLK_REG_CLK_SRC_SEL_SYS_CLK24M << APB1_CLK_REG_CLK_SRC_SEL_OFFSET | 0 << APB1_CLK_REG_FACTOR_M_OFFSET);
}

static inline void set_pll_nsi(void) {
    clrsetbits_le32(SUNXI_CCU_BASE + NSI_CLK_REG,
                    BIT(NSI_CLK_REG_NSI_CLK_GATING_OFFSET) | NSI_CLK_REG_NSI_DIV1_CLEAR_MASK,
                    (0x5 << NSI_CLK_REG_NSI_DIV1_OFFSET) | BIT(NSI_CLK_REG_NSI_UPD_OFFSET));
    while (read32(SUNXI_CCU_BASE + NSI_CLK_REG) & BIT(NSI_CLK_REG_NSI_UPD_OFFSET))
        ;
    clrsetbits_le32(SUNXI_CCU_BASE + NSI_CLK_REG,
                    NSI_CLK_REG_NSI_CLK_SEL_CLEAR_MASK,
                    (NSI_CLK_REG_NSI_CLK_SEL_PERI0_600M << NSI_CLK_REG_NSI_CLK_SEL_OFFSET) | BIT(NSI_CLK_REG_NSI_CLK_GATING_OFFSET) | BIT(NSI_CLK_REG_NSI_UPD_OFFSET));
    while (read32(SUNXI_CCU_BASE + NSI_CLK_REG) & BIT(NSI_CLK_REG_NSI_UPD_OFFSET))
        ;
}

void sunxi_clk_init(void) {
    set_pll_parent();
    set_pll_cpux_axi();
    set_apb1();
    set_pll_nsi();
}

uint32_t sunxi_clk_get_hosc_type() {
    if (read32(RTC_XO_CONTROL0_REG) & BIT(15)) {
        return 26;
    } else {
        return 24;
    }
}

void sunxi_clk_reset(void) {
}

void sunxi_clk_dump(void) {
}
