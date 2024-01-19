/* SPDX-License-Identifier:	GPL-2.0+ */

#ifndef __REG_USB_H__
#define __REG_USB_H__
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <reg-ncat.h>

#define CCU_USB0_CLK_REG 0x0A70       /* USB0 Clock Register */
#define CCU_USB_BGR_REG 0x0A8C        /* USB Bus Gating Reset Register */
#define CCU_CLK24M_GATE_EN_REG 0x0E0C /* CLK24M Gate Enable Register */

#define USB_PHY_SEL 0x0420
#define USB_PHY_CTL 0x410

#define USBC_REG_o_ISCR 0x0400
#define USBC_REG_o_PHYBIST 0x0408
#define USBC_REG_o_PHYTUNE 0x040c
#define USBC_REG_o_PHYCTL 0x0410
#define USBC_REG_o_SEL 0x420

#define USBC_REG_o_DMA_ENABLE 0x0500
#define USBC_REG_o_DMA_STATUS 0x0504
#define USBC_REG_o_DMA_CONFIG 0x0540
#define USBC_REG_o_DMA_ADDR 0x0544
#define USBC_REG_o_DMA_SIZE 0x0548
#define USBC_REG_o_DMA_RESU 0x0548

#define USBC_REG_FADDR(usbc_base_addr) ((usbc_base_addr) + USBC_REG_o_FADDR)
#define USBC_REG_PCTL(usbc_base_addr) ((usbc_base_addr) + USBC_REG_o_PCTL)
#define USBC_REG_INTTx(usbc_base_addr) ((usbc_base_addr) + USBC_REG_o_INTTx)
#define USBC_REG_INTRx(usbc_base_addr) ((usbc_base_addr) + USBC_REG_o_INTRx)
#define USBC_REG_INTTxE(usbc_base_addr) ((usbc_base_addr) + USBC_REG_o_INTTxE)
#define USBC_REG_INTRxE(usbc_base_addr) ((usbc_base_addr) + USBC_REG_o_INTRxE)
#define USBC_REG_INTUSB(usbc_base_addr) ((usbc_base_addr) + USBC_REG_o_INTUSB)
#define USBC_REG_INTUSBE(usbc_base_addr) ((usbc_base_addr) + USBC_REG_o_INTUSBE)
#define USBC_REG_FRNUM(usbc_base_addr) ((usbc_base_addr) + USBC_REG_o_FRNUM)
#define USBC_REG_EPIND(usbc_base_addr) ((usbc_base_addr) + USBC_REG_o_EPIND)
#define USBC_REG_TMCTL(usbc_base_addr) ((usbc_base_addr) + USBC_REG_o_TMCTL)
#define USBC_REG_TXMAXP(usbc_base_addr) ((usbc_base_addr) + USBC_REG_o_TXMAXP)

#define USBC_Readb(reg) (*(volatile uint8_t *) (reg))
#define USBC_Readw(reg) (*(volatile uint16_t *) (reg))
#define USBC_Readl(reg) (*(volatile uint32_t *) (reg))

#define USBC_Writeb(value, reg) (*(volatile uint8_t *) (reg) = (value))
#define USBC_Writew(value, reg) (*(volatile uint16_t *) (reg) = (value))
#define USBC_Writel(value, reg) (*(volatile uint32_t *) (reg) = (value))

#define USBC_REG_test_bit_b(bp, reg) (USBC_Readb(reg) & (1 << (bp)))
#define USBC_REG_test_bit_w(bp, reg) (USBC_Readw(reg) & (1 << (bp)))
#define USBC_REG_test_bit_l(bp, reg) (USBC_Readl(reg) & (1 << (bp)))

#define USBC_REG_set_bit_b(bp, reg) \
    (USBC_Writeb((USBC_Readb(reg) | (1 << (bp))), (reg)))
#define USBC_REG_set_bit_w(bp, reg) \
    (USBC_Writew((USBC_Readw(reg) | (1 << (bp))), (reg)))
#define USBC_REG_set_bit_l(bp, reg) \
    (USBC_Writel((USBC_Readl(reg) | (1 << (bp))), (reg)))

#define USBC_REG_clear_bit_b(bp, reg) \
    (USBC_Writeb((USBC_Readb(reg) & (~(1 << (bp)))), (reg)))
#define USBC_REG_clear_bit_w(bp, reg) \
    (USBC_Writew((USBC_Readw(reg) & (~(1 << (bp)))), (reg)))
#define USBC_REG_clear_bit_l(bp, reg) \
    (USBC_Writel((USBC_Readl(reg) & (~(1 << (bp)))), (reg)))

#endif// __REG_USB_H__