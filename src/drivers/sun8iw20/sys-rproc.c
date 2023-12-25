/* SPDX-License-Identifier: Apache-2.0 */

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>

#include <sys-clk.h>

#include <sys-rproc.h>

void sunxi_c906_clock_init(uint32_t addr) {
    uint32_t reg_val;

    reg_val = 0;
    reg_val |= CCU_RISCV_CFG_RST;
    reg_val |= CCU_RISCV_CFG_GATING;
    writel(reg_val, CCU_RISCV_CFG_BGR_REG);

    /* set start addr */
    reg_val = addr;
    writel(reg_val, CCU_RISCV_STA_ADD_L_REG);
    reg_val = 0;
    writel(reg_val, CCU_RISCV_STA_ADD_H_REG);

    /* set c906 clock */
    reg_val = readl(CCU_RISCV_CLK_REG);
    reg_val &= ~(CCU_RISCV_CLK_MASK);
    reg_val |= CCU_RISCV_CLK_PERI_800M;
    writel(reg_val, CCU_RISCV_CLK_REG);

    /* soft reset */
    reg_val = CCU_RISCV_RST_KEY_FIELD;
    reg_val |= CCU_RISCV_RST_SOFT_RSTN;
    writel(reg_val, CCU_RISCV_RST_REG);
}

void sunxi_c906_clock_reset(void) {
    uint32_t reg_val;

    reg_val = 0;
    reg_val |= CCU_RISCV_CLK_GATING;
    reg_val |= CCU_RISCV_GATING_FIELD;
    writel(reg_val, CCU_RISCV_GATING_RST_REG);

    reg_val = 0;
    writel(reg_val, CCU_RISCV_CFG_BGR_REG);
}

void dump_c906_clock(void) {
    // TODO
}