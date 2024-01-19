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

    reg_val = readl(SUNXI_CCU_BASE + CCU_USB_BGR_REG);
    reg_val |= (1 << 0);
    writel(reg_val, (SUNXI_CCU_BASE + CCU_USB_BGR_REG));

    reg_val = readl(SUNXI_CCU_BASE + CCU_USB_BGR_REG);
    reg_val |= (1 << 24);
    writel(reg_val, (SUNXI_CCU_BASE + CCU_USB_BGR_REG));


    reg_val = readl(SUNXI_CCU_BASE + CCU_USB_BGR_REG);
    reg_val |= (1 << 8);
    writel(reg_val, (SUNXI_CCU_BASE + CCU_USB_BGR_REG));

    reg_val = readl(SUNXI_CCU_BASE + CCU_USB0_CLK_REG);
    reg_val |= (1 << 29);
    writel(reg_val, (SUNXI_CCU_BASE + CCU_USB0_CLK_REG));

    reg_val = readl(SUNXI_CCU_BASE + CCU_USB0_CLK_REG);
    reg_val |= (1 << 30);
    writel(reg_val, (SUNXI_CCU_BASE + CCU_USB0_CLK_REG));

    reg_val = readl(SUNXI_USB0_BASE + USB_PHY_SEL);
    reg_val |= (0x01 << 0);
    writel(reg_val, (SUNXI_USB0_BASE + USB_PHY_SEL));

    reg_val = readl((SUNXI_USB0_BASE + USB_PHY_CTL));
    reg_val &= ~(0x01 << 3);
    reg_val |= (0x01 << 5);
    writel(reg_val, (SUNXI_USB0_BASE + USB_PHY_CTL));
}

void sunxi_usb_clk_deinit(void) {
    uint32_t reg_val = 0;

    reg_val = readl(SUNXI_CCU_BASE + CCU_USB0_CLK_REG);
    reg_val &= ~(1 << 29);
    writel(reg_val, (SUNXI_CCU_BASE + CCU_USB0_CLK_REG));

    reg_val = readl(SUNXI_CCU_BASE + CCU_USB0_CLK_REG);
    reg_val &= ~(1 << 30);
    writel(reg_val, (SUNXI_CCU_BASE + CCU_USB0_CLK_REG));

    reg_val = readl(SUNXI_CCU_BASE + CCU_USB_BGR_REG);
    reg_val &= ~(1 << 24);
    writel(reg_val, (SUNXI_CCU_BASE + CCU_USB_BGR_REG));

    reg_val = readl(SUNXI_CCU_BASE + CCU_USB_BGR_REG);
    reg_val &= ~(1 << 8);
    writel(reg_val, (SUNXI_CCU_BASE + CCU_USB_BGR_REG));

    reg_val = readl(SUNXI_CCU_BASE + CCU_CLK24M_GATE_EN_REG);
    reg_val &= ~(1 << 0);
    writel(reg_val, (SUNXI_CCU_BASE + CCU_CLK24M_GATE_EN_REG));
}
