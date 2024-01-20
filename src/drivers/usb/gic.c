/* SPDX-License-Identifier:	GPL-2.0+ */

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <reg-ncat.h>

#define GIC_DIST_BASE (SUNXI_GIC_BASE + 0x1000)
#define GIC_SET_EN(n) (GIC_DIST_BASE + 0x100 + 4 * (n))

#define GIC_CPUIF_BASE (SUNXI_GIC_BASE + 0x2000)
#define GIC_INT_ACK_REG (GIC_CPUIF_BASE + 0x00c)// 0x800c

void gic_enable(uint32_t irq) {
    uint32_t reg_val, offset;

    offset = irq >> 5;
    reg_val = readl(GIC_SET_EN(offset));
    reg_val |= 1 << (irq & 0x1f);
    writel(reg_val, GIC_SET_EN(offset));
}

#define GIC_PEND_SET(n) (GIC_DIST_BASE + 0x200 + 4 * (n))

int gic_is_pending(uint32_t irq) {
    uint32_t reg_val, offset;

    offset = irq >> 5;
    reg_val = readl(GIC_PEND_SET(offset));
    return reg_val & BIT(irq & 0x1f);
}
