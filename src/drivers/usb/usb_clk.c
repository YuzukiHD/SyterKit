/* SPDX-License-Identifier:	GPL-2.0+ */

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <common.h>
#include <log.h>

#include <sys-clk.h>

#include "usb.h"

void sunxi_usb_clk_init(void) {
    uint32_t reg_val = 0;

    /* USB0 Clock Reg */
    reg_val = readl(SUNXI_CCU_BASE + CCU_USB0_CLK_REG);
    reg_val |= (1 << 31);
    writel(reg_val, (SUNXI_CCU_BASE + CCU_USB0_CLK_REG));

    /* Delay for some time */
    mdelay(1);

    /* bit30: USB PHY0 reset */
    /* Bit29: Gating Special Clk for USB PHY0 */
    reg_val = readl(SUNXI_CCU_BASE + CCU_USB0_CLK_REG);
    reg_val |= (1 << 30);
    writel(reg_val, (SUNXI_CCU_BASE + CCU_USB0_CLK_REG));

    /* Delay for some time */
    mdelay(1);

    /* USB BUS Gating Reset Reg: USB_OTG reset */
    reg_val = readl(SUNXI_CCU_BASE + CCU_USB_BGR_REG);
    reg_val |= (1 << 24);
    writel(reg_val, (SUNXI_CCU_BASE + CCU_USB_BGR_REG));
    mdelay(1);

    /* USB BUS Gating Reset Reg */
    /* bit8:USB_OTG Gating */
    reg_val = readl(SUNXI_CCU_BASE + CCU_USB_BGR_REG);
    reg_val |= (1 << 8);
    writel(reg_val, (SUNXI_CCU_BASE + CCU_USB_BGR_REG));

    /* Delay to wait for SIE stability */
    mdelay(1);
}

void sunxi_usb_clk_deinit(void) {
    uint32_t reg_val = 0;
    
    /* USB BUS Gating Reset Reg: USB_OTG reset */
    reg_val = readl(SUNXI_CCU_BASE + CCU_USB_BGR_REG);
    reg_val &= ~(1 << 24);
    writel(reg_val, (SUNXI_CCU_BASE + CCU_USB_BGR_REG));
    mdelay(1);

    /* USB BUS Gating Reset Reg */
    /* bit8:USB_OTG Gating */
    reg_val = readl(SUNXI_CCU_BASE + CCU_USB_BGR_REG);
    reg_val &= ~(1 << 8);
    writel(reg_val, (SUNXI_CCU_BASE + CCU_USB_BGR_REG));
    mdelay(1);
}
