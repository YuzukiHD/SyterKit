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

void sunxi_hifi4_clock_init(uint32_t addr) {

}

void sunxi_hifi4_clock_reset(void) {

}

void dump_hifi4_clock(void) {
    uint32_t reg_val, pll_perf, factor_m, factor_n, pll_riscv;
    uint32_t plln, pllm;
    uint8_t p0, p1;

    /* PLL PERI0 */
    reg_val = read32(CCU_BASE + CCU_PLL_PERI0_CTRL_REG);

    if (reg_val & (1 << 31)) {
        plln = ((reg_val >> 8) & 0xff) + 1;
        pllm = (reg_val & 0x01) + 1;
        p0 = ((reg_val >> 16) & 0x03) + 1;
        p1 = ((reg_val >> 20) & 0x03) + 1;

        pll_perf = (24 * plln) / (pllm * p1);
    } else {
        printk(LOG_LEVEL_INFO, "CLK: PLL_peri disabled\n");
        return;
    }

    reg_val = read32(CCU_RISCV_CLK_REG);
    factor_m = (reg_val & 0x1F) + 1;
    factor_n = ((reg_val >> 8) & 0x3) + 1;
    pll_riscv = pll_perf / factor_m;

    printk(LOG_LEVEL_INFO, "CLK: HIFI4 PLL FREQ=%uMHz\n", pll_riscv);
    printk(LOG_LEVEL_INFO, "CLK: HIFI4 AXI FREQ=%uMHz\n",
           pll_riscv / factor_n);
}