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

static void clock_set_pll_cpu(uint32_t clk) {
    int p = 0;
    int k = 1;
    int m = 1;
    uint32_t val;

    if (clk > 1152000000) {
        k = 2;
    } else if (clk > 768000000) {
        k = 3;
        m = 2;
    }

    /* Switch to 24MHz clock while changing cpu pll */
    val = (2 << 0) | (1 << 8) | (1 << 16);
    write32(CCU_BASE + CCU_CPU_AXI_CFG, val);

    /* cpu pll rate = ((24000000 * n * k) >> p) / m */
    val = (0x1 << 31);
    val |= ((p & 0x3) << 16);
    val |= ((((clk / (24000000 * k / m)) - 1) & 0x1f) << 8);
    val |= (((k - 1) & 0x3) << 4);
    val |= (((m - 1) & 0x3) << 0);
    write32(CCU_BASE + CCU_PLL_CPU_CTRL, val);
    sdelay(200);

    /* Switch clock source */
    val = (2 << 0) | (1 << 8) | (2 << 16);
    write32(CCU_BASE + CCU_CPU_AXI_CFG, val);
}

void sunxi_clk_init(void) {
    clock_set_pll_cpu(1008000000);

    /* pll video - 396MHZ */
    write32(CCU_BASE + CCU_PLL_VIDEO_CTRL, 0x91004107);

    /* pll periph0 - 600MHZ */
    write32(CCU_BASE + CCU_PLL_PERIPH0_CTRL, 0x90041811);
    while (!(read32(CCU_BASE + CCU_PLL_PERIPH0_CTRL) & (1 << 28)))
        ;

    /* ahb1 = pll periph0 / 3, apb1 = ahb1 / 2 */
    write32(CCU_BASE + CCU_AHB_APB0_CFG, 0x00003180);

    /* mbus  = pll periph0 / 4 */
    write32(CCU_BASE + CCU_MBUS_CLK, 0x81000003);
}

void sunxi_clk_reset(void) {
    return;
}

void sunxi_clk_dump(void) {
    return;
}

uint32_t sunxi_clk_get_peri1x_rate() {
    return 0;
}