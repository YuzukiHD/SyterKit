/* SPDX-License-Identifier:	GPL-2.0+ */

#ifndef __DRIVERS_USB_USB_REGS_H__
#define __DRIVERS_USB_USB_REGS_H__

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <io.h>

/* USB PHY Define */
#define USB_PHY_SEL 0x0420
#define USB_PHY_CTL 0x0410

/* reg offset */
#define USBC_REG_o_FADDR 0x0098
#define USBC_REG_o_PCTL 0x0040
#define USBC_REG_o_INTTx 0x0044
#define USBC_REG_o_INTRx 0x0046
#define USBC_REG_o_INTTxE 0x0048
#define USBC_REG_o_INTRxE 0x004A
#define USBC_REG_o_INTUSB 0x004C
#define USBC_REG_o_INTUSBE 0x0050
#define USBC_REG_o_FRNUM 0x0054
#define USBC_REG_o_EPIND 0x0042
#define USBC_REG_o_TMCTL 0x007C

#define USBC_REG_o_TXMAXP 0x0080
#define USBC_REG_o_CSR0 0x0082
#define USBC_REG_o_TXCSR 0x0082
#define USBC_REG_o_RXMAXP 0x0084
#define USBC_REG_o_RXCSR 0x0086
#define USBC_REG_o_COUNT0 0x0088
#define USBC_REG_o_RXCOUNT 0x0088
#define USBC_REG_o_EP0TYPE 0x008C
#define USBC_REG_o_TXTYPE 0x008C
#define USBC_REG_o_NAKLIMIT0 0x008D
#define USBC_REG_o_TXINTERVAL 0x008D
#define USBC_REG_o_RXTYPE 0x008E
#define USBC_REG_o_RXINTERVAL 0x008F

#define USBC_REG_o_CONFIGDATA 0x00c0 /**/

#define USBC_REG_o_EPFIFO0 0x0000
#define USBC_REG_o_EPFIFO1 0x0004
#define USBC_REG_o_EPFIFO2 0x0008
#define USBC_REG_o_EPFIFO3 0x000C
#define USBC_REG_o_EPFIFO4 0x0010
#define USBC_REG_o_EPFIFO5 0x0014
static inline uintptr_t USBC_REG_o_EPFIFOx(uint32_t n)
{
	return (uintptr_t)(n << 2);
}

#define USBC_REG_o_DEVCTL 0x0041

#define USBC_REG_o_TXFIFOSZ 0x0090
#define USBC_REG_o_RXFIFOSZ 0x0094
#define USBC_REG_o_TXFIFOAD 0x0092
#define USBC_REG_o_RXFIFOAD 0x0096

#define USBC_REG_o_VEND0 0x0043
#define USBC_REG_o_VEND1 0x007D
#define USBC_REG_o_VEND3 0x007E

/*#define  USBC_REG_o_PHYCTL		0x006C*/
#define USBC_REG_o_EPINFO 0x0078
#define USBC_REG_o_RAMINFO 0x0079
#define USBC_REG_o_LINKINFO 0x007A
#define USBC_REG_o_VPLEN 0x007B
#define USBC_REG_o_HSEOF 0x007C
#define USBC_REG_o_FSEOF 0x007D
#define USBC_REG_o_LSEOF 0x007E

/*new*/
#define USBC_REG_o_FADDR0 0x0098
#define USBC_REG_o_HADDR0 0x009A
#define USBC_REG_o_HPORT0 0x009B
#define USBC_REG_o_TXFADDRx 0x0098
#define USBC_REG_o_TXHADDRx 0x009A
#define USBC_REG_o_TXHPORTx 0x009B
#define USBC_REG_o_RXFADDRx 0x009C
#define USBC_REG_o_RXHADDRx 0x009E
#define USBC_REG_o_RXHPORTx 0x009F

#define USBC_REG_o_RPCOUNT 0x008A

/*new*/
#define USBC_REG_o_ISCR 0x0400
/*#define  USBC_REG_o_PHYCTL          0x0404*/
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
/*-----------------------------------------------------------------------*/
/*   registers*/
/*-----------------------------------------------------------------------*/

static inline uintptr_t USBC_REG_FADDR(uintptr_t base)
{
	return base + USBC_REG_o_FADDR;
}

static inline uintptr_t USBC_REG_PCTL(uintptr_t base)
{
	return base + USBC_REG_o_PCTL;
}

static inline uintptr_t USBC_REG_INTTx(uintptr_t base)
{
	return base + USBC_REG_o_INTTx;
}

static inline uintptr_t USBC_REG_INTRx(uintptr_t base)
{
	return base + USBC_REG_o_INTRx;
}

static inline uintptr_t USBC_REG_INTTxE(uintptr_t base)
{
	return base + USBC_REG_o_INTTxE;
}

static inline uintptr_t USBC_REG_INTRxE(uintptr_t base)
{
	return base + USBC_REG_o_INTRxE;
}

static inline uintptr_t USBC_REG_INTUSB(uintptr_t base)
{
	return base + USBC_REG_o_INTUSB;
}

static inline uintptr_t USBC_REG_INTUSBE(uintptr_t base)
{
	return base + USBC_REG_o_INTUSBE;
}

static inline uintptr_t USBC_REG_FRNUM(uintptr_t base)
{
	return base + USBC_REG_o_FRNUM;
}

static inline uintptr_t USBC_REG_EPIND(uintptr_t base)
{
	return base + USBC_REG_o_EPIND;
}

static inline uintptr_t USBC_REG_TMCTL(uintptr_t base)
{
	return base + USBC_REG_o_TMCTL;
}

static inline uintptr_t USBC_REG_TXMAXP(uintptr_t base)
{
	return base + USBC_REG_o_TXMAXP;
}

static inline uintptr_t USBC_REG_CSR0(uintptr_t base)
{
	return base + USBC_REG_o_CSR0;
}

static inline uintptr_t USBC_REG_TXCSR(uintptr_t base)
{
	return base + USBC_REG_o_TXCSR;
}

static inline uintptr_t USBC_REG_RXMAXP(uintptr_t base)
{
	return base + USBC_REG_o_RXMAXP;
}

static inline uintptr_t USBC_REG_RXCSR(uintptr_t base)
{
	return base + USBC_REG_o_RXCSR;
}

static inline uintptr_t USBC_REG_COUNT0(uintptr_t base)
{
	return base + USBC_REG_o_COUNT0;
}

static inline uintptr_t USBC_REG_RXCOUNT(uintptr_t base)
{
	return base + USBC_REG_o_RXCOUNT;
}

static inline uintptr_t USBC_REG_EP0TYPE(uintptr_t base)
{
	return base + USBC_REG_o_EP0TYPE;
}

static inline uintptr_t USBC_REG_TXTYPE(uintptr_t base)
{
	return base + USBC_REG_o_TXTYPE;
}

static inline uintptr_t USBC_REG_NAKLIMIT0(uintptr_t base)
{
	return base + USBC_REG_o_NAKLIMIT0;
}

static inline uintptr_t USBC_REG_TXINTERVAL(uintptr_t base)
{
	return base + USBC_REG_o_TXINTERVAL;
}

static inline uintptr_t USBC_REG_RXTYPE(uintptr_t base)
{
	return base + USBC_REG_o_RXTYPE;
}

static inline uintptr_t USBC_REG_RXINTERVAL(uintptr_t base)
{
	return base + USBC_REG_o_RXINTERVAL;
}

static inline uintptr_t USBC_REG_CONFIGDATA(uintptr_t base)
{
	return base + USBC_REG_o_CONFIGDATA;
}

static inline uintptr_t USBC_REG_EPFIFO0(uintptr_t base)
{
	return base + USBC_REG_o_EPFIFO0;
}

static inline uintptr_t USBC_REG_EPFIFO1(uintptr_t base)
{
	return base + USBC_REG_o_EPFIFO1;
}

static inline uintptr_t USBC_REG_EPFIFO2(uintptr_t base)
{
	return base + USBC_REG_o_EPFIFO2;
}

static inline uintptr_t USBC_REG_EPFIFO3(uintptr_t base)
{
	return base + USBC_REG_o_EPFIFO3;
}

static inline uintptr_t USBC_REG_EPFIFO4(uintptr_t base)
{
	return base + USBC_REG_o_EPFIFO4;
}

static inline uintptr_t USBC_REG_EPFIFO5(uintptr_t base)
{
	return base + USBC_REG_o_EPFIFO5;
}

static inline uintptr_t USBC_REG_EPFIFOx(uintptr_t base, uint32_t n)
{
	return base + USBC_REG_o_EPFIFOx(n);
}

static inline uintptr_t USBC_REG_DEVCTL(uintptr_t base)
{
	return base + USBC_REG_o_DEVCTL;
}

static inline uintptr_t USBC_REG_TXFIFOSZ(uintptr_t base)
{
	return base + USBC_REG_o_TXFIFOSZ;
}

static inline uintptr_t USBC_REG_RXFIFOSZ(uintptr_t base)
{
	return base + USBC_REG_o_RXFIFOSZ;
}

static inline uintptr_t USBC_REG_TXFIFOAD(uintptr_t base)
{
	return base + USBC_REG_o_TXFIFOAD;
}

static inline uintptr_t USBC_REG_RXFIFOAD(uintptr_t base)
{
	return base + USBC_REG_o_RXFIFOAD;
}

static inline uintptr_t USBC_REG_VEND0(uintptr_t base)
{
	return base + USBC_REG_o_VEND0;
}

static inline uintptr_t USBC_REG_VEND1(uintptr_t base)
{
	return base + USBC_REG_o_VEND1;
}

static inline uintptr_t USBC_REG_EPINFO(uintptr_t base)
{
	return base + USBC_REG_o_EPINFO;
}

static inline uintptr_t USBC_REG_RAMINFO(uintptr_t base)
{
	return base + USBC_REG_o_RAMINFO;
}

static inline uintptr_t USBC_REG_LINKINFO(uintptr_t base)
{
	return base + USBC_REG_o_LINKINFO;
}

static inline uintptr_t USBC_REG_VPLEN(uintptr_t base)
{
	return base + USBC_REG_o_VPLEN;
}

static inline uintptr_t USBC_REG_HSEOF(uintptr_t base)
{
	return base + USBC_REG_o_HSEOF;
}

static inline uintptr_t USBC_REG_FSEOF(uintptr_t base)
{
	return base + USBC_REG_o_FSEOF;
}

static inline uintptr_t USBC_REG_LSEOF(uintptr_t base)
{
	return base + USBC_REG_o_LSEOF;
}

static inline uintptr_t USBC_REG_FADDR0(uintptr_t base)
{
	return base + USBC_REG_o_FADDR0;
}

static inline uintptr_t USBC_REG_HADDR0(uintptr_t base)
{
	return base + USBC_REG_o_HADDR0;
}

static inline uintptr_t USBC_REG_HPORT0(uintptr_t base)
{
	return base + USBC_REG_o_HPORT0;
}

static inline uintptr_t USBC_REG_TXFADDRx(uintptr_t base, uint32_t n)
{
	(void)n;
	return base + USBC_REG_o_TXFADDRx;
}

static inline uintptr_t USBC_REG_TXHADDRx(uintptr_t base, uint32_t n)
{
	(void)n;
	return base + USBC_REG_o_TXHADDRx;
}

static inline uintptr_t USBC_REG_TXHPORTx(uintptr_t base, uint32_t n)
{
	(void)n;
	return base + USBC_REG_o_TXHPORTx;
}

static inline uintptr_t USBC_REG_RXFADDRx(uintptr_t base, uint32_t n)
{
	(void)n;
	return base + USBC_REG_o_RXFADDRx;
}

static inline uintptr_t USBC_REG_RXHADDRx(uintptr_t base, uint32_t n)
{
	(void)n;
	return base + USBC_REG_o_RXHADDRx;
}

static inline uintptr_t USBC_REG_RXHPORTx(uintptr_t base, uint32_t n)
{
	(void)n;
	return base + USBC_REG_o_RXHPORTx;
}

static inline uintptr_t USBC_REG_RPCOUNTx(uintptr_t base, uint32_t n)
{
	(void)n;
	return base + USBC_REG_o_RPCOUNT;
}

static inline uintptr_t USBC_REG_ISCR(uintptr_t base)
{
	return base + USBC_REG_o_ISCR;
}

static inline uintptr_t USBC_REG_PHYCTL(uintptr_t base)
{
	return base + USBC_REG_o_PHYCTL;
}

static inline uintptr_t USBC_REG_PHYBIST(uintptr_t base)
{
	return base + USBC_REG_o_PHYBIST;
}

static inline uintptr_t USBC_REG_PHYTUNE(uintptr_t base)
{
	return base + USBC_REG_o_PHYTUNE;
}

static inline uintptr_t USBC_REG_RXFADDRx_Ex(uintptr_t base)
{
	return base + USBC_REG_o_RXFADDRx;
}

/*DMA*/
#define USBC_REG_o_DMA_INTE 0x0500
#define USBC_REG_o_DMA_INTS 0x0504
static inline uintptr_t USBC_REG_o_DMA_CHAN_CFN(uint32_t n)
{
	return 0x0540U + (0x10U * n);
}

static inline uintptr_t USBC_REG_o_DMA_SDRAM_ADD(uint32_t n)
{
	return 0x0544U + (0x10U * n);
}

static inline uintptr_t USBC_REG_o_DMA_BC(uint32_t n)
{
	return 0x0548U + (0x10U * n);
}

static inline uintptr_t USBC_REG_o_DMA_RESIDUAL_BC(uint32_t n)
{
	return 0x0548U + (0x10U * n);
}

static inline uintptr_t USBC_REG_DMA_INTE(uintptr_t base)
{
	return base + USBC_REG_o_DMA_INTE;
}

static inline uintptr_t USBC_REG_DMA_INTS(uintptr_t base)
{
	return base + USBC_REG_o_DMA_INTS;
}

static inline uintptr_t USBC_REG_DMA_CHAN_CFN(uintptr_t base, uint32_t n)
{
	return base + USBC_REG_o_DMA_CHAN_CFN(n);
}

static inline uintptr_t USBC_REG_DMA_SDRAM_ADD(uintptr_t base, uint32_t n)
{
	return base + USBC_REG_o_DMA_SDRAM_ADD(n);
}

static inline uintptr_t USBC_REG_DMA_BC(uintptr_t base, uint32_t n)
{
	return base + USBC_REG_o_DMA_BC(n);
}

static inline uintptr_t USBC_REG_DMA_RESIDUAL_BC(uintptr_t base, uint32_t n)
{
	return base + USBC_REG_o_DMA_RESIDUAL_BC(n);
}

/*-----------------------------------------------------------------------*/
/*  registers extern*/
/*-----------------------------------------------------------------------*/
static inline uintptr_t USBC_REG_EX_USB_EPFIFOn(uintptr_t base, uint32_t n)
{
	return base + (n << 2);
}

static inline uintptr_t USBC_REG_EX_USB_GCS(uintptr_t base)
{
	return base + 0x0040U;
}

static inline uintptr_t USBC_REG_EX_USB_EPINTF(uintptr_t base)
{
	return base + 0x0044U;
}

static inline uintptr_t USBC_REG_EX_USB_EPINTE(uintptr_t base)
{
	return base + 0x0048U;
}

static inline uintptr_t USBC_REG_EX_USB_BUSINTF(uintptr_t base)
{
	return base + 0x004cU;
}

static inline uintptr_t USBC_REG_EX_USB_BUSINTE(uintptr_t base)
{
	return base + 0x0050U;
}

static inline uintptr_t USBC_REG_EX_USB_FNUM(uintptr_t base)
{
	return base + 0x0054U;
}

static inline uintptr_t USBC_REG_EX_USB_TESTC(uintptr_t base)
{
	return base + 0x007cU;
}

static inline uintptr_t USBC_REG_EX_USB_CSR0(uintptr_t base)
{
	return base + 0x0080U;
}

static inline uintptr_t USBC_REG_EX_USB_TXCSR(uintptr_t base)
{
	return base + 0x0080U;
}

static inline uintptr_t USBC_REG_EX_USB_RXCSR(uintptr_t base)
{
	return base + 0x0084U;
}

static inline uintptr_t USBC_REG_EX_USB_COUNT0(uintptr_t base)
{
	return base + 0x0088U;
}

static inline uintptr_t USBC_REG_EX_USB_RXCOUNT(uintptr_t base)
{
	return base + 0x0088U;
}

static inline uintptr_t USBC_REG_EX_USB_ATTR0(uintptr_t base)
{
	return base + 0x008cU;
}

static inline uintptr_t USBC_REG_EX_USB_EPATTR(uintptr_t base)
{
	return base + 0x008cU;
}

static inline uintptr_t USBC_REG_EX_USB_TXFIFO(uintptr_t base)
{
	return base + 0x0090U;
}

static inline uintptr_t USBC_REG_EX_USB_RXFIFO(uintptr_t base)
{
	return base + 0x0094U;
}

static inline uintptr_t USBC_REG_EX_USB_FADDR(uintptr_t base)
{
	return base + 0x0098U;
}

static inline uintptr_t USBC_REG_EX_USB_TXFADDR(uintptr_t base)
{
	return base + 0x0098U;
}

static inline uintptr_t USBC_REG_EX_USB_RXFADDR(uintptr_t base)
{
	return base + 0x009cU;
}

static inline uintptr_t USBC_REG_EX_USB_CONFIGINFO(uintptr_t base)
{
	return base + 0x00c0U;
}

static inline uintptr_t USBC_REG_EX_USB_LINKTIM(uintptr_t base)
{
	return base + 0x00c4U;
}

static inline uintptr_t USBC_REG_EX_USB_OTGTIM(uintptr_t base)
{
	return base + 0x00c8U;
}

static inline uintptr_t USBC_REG_EX_USB_ISCR(uintptr_t base)
{
	return base + 0x0400U;
}

static inline uintptr_t USBC_REG_EX_USB_PHYCTL(uintptr_t base)
{
	return base + 0x0404U;
}

static inline uintptr_t USBC_REG_EX_USB_PHYBIST(uintptr_t base)
{
	return base + 0x0408U;
}

/*-----------------------------------------------------------------------*/
/*   bit position*/
/*-----------------------------------------------------------------------*/

/* USB Power Control for Host only  */
#define USBC_BP_POWER_H_HIGH_SPEED_EN 5
#define USBC_BP_POWER_H_HIGH_SPEED_FLAG 4
#define USBC_BP_POWER_H_RESET 3
#define USBC_BP_POWER_H_RESUME 2
#define USBC_BP_POWER_H_SUSPEND 1
#define USBC_BP_POWER_H_SUEPEND_EN 0

/* USB Power Control for device only  */
#define USBC_BP_POWER_D_ISO_UPDATE_EN 7
#define USBC_BP_POWER_D_SOFT_CONNECT 6
#define USBC_BP_POWER_D_HIGH_SPEED_EN 5
#define USBC_BP_POWER_D_HIGH_SPEED_FLAG 4
#define USBC_BP_POWER_D_RESET_FLAG 3
#define USBC_BP_POWER_D_RESUME 2
#define USBC_BP_POWER_D_SUSPEND 1
#define USBC_BP_POWER_D_ENABLE_SUSPENDM 0

/* interrupt flags for ep0 and the Tx ep1~4 */
#define USBC_BP_INTTx_FLAG_EP5 5
#define USBC_BP_INTTx_FLAG_EP4 4
#define USBC_BP_INTTx_FLAG_EP3 3
#define USBC_BP_INTTx_FLAG_EP2 2
#define USBC_BP_INTTx_FLAG_EP1 1
#define USBC_BP_INTTx_FLAG_EP0 0

/* interrupt flags for Rx ep1~4 */
#define USBC_BP_INTRx_FLAG_EP5 5
#define USBC_BP_INTRx_FLAG_EP4 4
#define USBC_BP_INTRx_FLAG_EP3 3
#define USBC_BP_INTRx_FLAG_EP2 2
#define USBC_BP_INTRx_FLAG_EP1 1

/* interrupt enable for Tx ep0~4 */
#define USBC_BP_INTTxE_EN_EP5 5
#define USBC_BP_INTTxE_EN_EP4 4
#define USBC_BP_INTTxE_EN_EP3 3
#define USBC_BP_INTTxE_EN_EP2 2
#define USBC_BP_INTTxE_EN_EP1 1
#define USBC_BP_INTTxE_EN_EP0 0

/* interrupt enable for Rx ep1~4 */
#define USBC_BP_INTRxE_EN_EP5 5
#define USBC_BP_INTRxE_EN_EP4 4
#define USBC_BP_INTRxE_EN_EP3 3
#define USBC_BP_INTRxE_EN_EP2 2
#define USBC_BP_INTRxE_EN_EP1 1

/* USB interrupt */
#define USBC_BP_INTUSB_VBUS_ERROR 7
#define USBC_BP_INTUSB_SESSION_REQ 6
#define USBC_BP_INTUSB_DISCONNECT 5
#define USBC_BP_INTUSB_CONNECT 4
#define USBC_BP_INTUSB_SOF 3
#define USBC_BP_INTUSB_RESET 2
#define USBC_BP_INTUSB_RESUME 1
#define USBC_BP_INTUSB_SUSPEND 0

/* USB interrupt enable */
#define USBC_BP_INTUSBE_EN_VBUS_ERROR 7
#define USBC_BP_INTUSBE_EN_SESSION_REQ 6
#define USBC_BP_INTUSBE_EN_DISCONNECT 5
#define USBC_BP_INTUSBE_EN_CONNECT 4
#define USBC_BP_INTUSBE_EN_SOF 3
#define USBC_BP_INTUSBE_EN_RESET 2
#define USBC_BP_INTUSBE_EN_RESUME 1
#define USBC_BP_INTUSBE_EN_SUSPEND 0

/* Test Mode Control */
#define USBC_BP_TMCTL_FORCE_HOST 7
#define USBC_BP_TMCTL_FIFO_ACCESS 6
#define USBC_BP_TMCTL_FORCE_FS 5
#define USBC_BP_TMCTL_FORCE_HS 4
#define USBC_BP_TMCTL_TEST_PACKET 3
#define USBC_BP_TMCTL_TEST_K 2
#define USBC_BP_TMCTL_TEST_J 1
#define USBC_BP_TMCTL_TEST_SE0_NAK 0

/* Tx Max packet */
#define USBC_BP_TXMAXP_PACKET_COUNT 11
#define USBC_BP_TXMAXP_MAXIMUM_PAYLOAD 0

/* Control and Status Register for ep0 for Host only */
#define USBC_BP_CSR0_H_DisPing 11
#define USBC_BP_CSR0_H_FlushFIFO 8
#define USBC_BP_CSR0_H_NAK_Timeout 7
#define USBC_BP_CSR0_H_StatusPkt 6
#define USBC_BP_CSR0_H_ReqPkt 5
#define USBC_BP_CSR0_H_Error 4
#define USBC_BP_CSR0_H_SetupPkt 3
#define USBC_BP_CSR0_H_RxStall 2
#define USBC_BP_CSR0_H_TxPkRdy 1
#define USBC_BP_CSR0_H_RxPkRdy 0

/* Control and Status Register for ep0 for device only */
#define USBC_BP_CSR0_D_FLUSH_FIFO 8
#define USBC_BP_CSR0_D_SERVICED_SETUP_END 7
#define USBC_BP_CSR0_D_SERVICED_RX_PKT_READY 6
#define USBC_BP_CSR0_D_SEND_STALL 5
#define USBC_BP_CSR0_D_SETUP_END 4
#define USBC_BP_CSR0_D_DATA_END 3
#define USBC_BP_CSR0_D_SENT_STALL 2
#define USBC_BP_CSR0_D_TX_PKT_READY 1
#define USBC_BP_CSR0_D_RX_PKT_READY 0

/* Tx ep Control and Status Register for Host only */
#define USBC_BP_TXCSR_H_AUTOSET 15
#define USBC_BP_TXCSR_H_RESERVED 14
#define USBC_BP_TXCSR_H_MODE 13
#define USBC_BP_TXCSR_H_DMA_REQ_EN 12
#define USBC_BP_TXCSR_H_FORCE_DATA_TOGGLE 11
#define USBC_BP_TXCSR_H_DMA_REQ_MODE 10
#define USBC_BP_TXCSR_H_DATA_TOGGLE_WR_EN 9
#define USBC_BP_TXCSR_H_DATA_TOGGLE 8
#define USBC_BP_TXCSR_H_NAK_TIMEOUT 7
#define USBC_BP_TXCSR_H_CLEAR_DATA_TOGGLE 6
#define USBC_BP_TXCSR_H_TX_STALL 5
#define USBC_BP_TXCSR_H_FLUSH_FIFO 3
#define USBC_BP_TXCSR_H_ERROR 2
#define USBC_BP_TXCSR_H_FIFO_NOT_EMPTY 1
#define USBC_BP_TXCSR_H_TX_READY 0

#define USBC_TXCSR_H_WZC_BITS ((1 << USBC_BP_TXCSR_H_NAK_TIMEOUT) | (1 << USBC_BP_TXCSR_H_TX_STALL) | (1 << USBC_BP_TXCSR_H_ERROR) | (1 << USBC_BP_TXCSR_H_FIFO_NOT_EMPTY))

/* Tx ep Control and Status Register for Device only */
#define USBC_BP_TXCSR_D_AUTOSET 15
#define USBC_BP_TXCSR_D_ISO 14
#define USBC_BP_TXCSR_D_MODE 13
#define USBC_BP_TXCSR_D_DMA_REQ_EN 12
#define USBC_BP_TXCSR_D_FORCE_DATA_TOGGLE 11
#define USBC_BP_TXCSR_D_DMA_REQ_MODE 10
#define USBC_BP_TXCSR_D_INCOMPLETE 7
#define USBC_BP_TXCSR_D_CLEAR_DATA_TOGGLE 6
#define USBC_BP_TXCSR_D_SENT_STALL 5
#define USBC_BP_TXCSR_D_SEND_STALL 4
#define USBC_BP_TXCSR_D_FLUSH_FIFO 3
#define USBC_BP_TXCSR_D_UNDER_RUN 2
#define USBC_BP_TXCSR_D_FIFO_NOT_EMPTY 1
#define USBC_BP_TXCSR_D_TX_READY 0

/* Rx Max Packet */
#define USBC_BP_RXMAXP_PACKET_COUNT 11
#define USBC_BP_RXMAXP_MAXIMUM_PAYLOAD 0

/* Rx ep Control and Status Register for Host only */
#define USBC_BP_RXCSR_H_AUTO_CLEAR 15
#define USBC_BP_RXCSR_H_AUTO_REQ 14
#define USBC_BP_RXCSR_H_DMA_REQ_EN 13
#define USBC_BP_RXCSR_H_DISNYET 12	 /* dis nyet */
#define USBC_BP_RXCSR_H_PID_ERROR 12 /* pid error */
#define USBC_BP_RXCSR_H_DMA_REQ_MODE 11
#define USBC_BP_RXCSR_H_DATA_TOGGLE_WR_EN 10
#define USBC_BP_RXCSR_H_DATA_TOGGLE 9
#define USBC_BP_RXCSR_H_INCOMPLETE 8
#define USBC_BP_RXCSR_H_CLEAR_DATA_TOGGLE 7
#define USBC_BP_RXCSR_H_RX_STALL 6
#define USBC_BP_RXCSR_H_REQ_PACKET 5
#define USBC_BP_RXCSR_H_FLUSH_FIFO 4
#define USBC_BP_RXCSR_H_DATA_ERR 3	  /* iso */
#define USBC_BP_RXCSR_H_NAK_TIMEOUT 3 /* bulk */
#define USBC_BP_RXCSR_H_ERROR 2
#define USBC_BP_RXCSR_H_FIFO_FULL 1
#define USBC_BP_RXCSR_H_RX_PKT_READY 0

#define USBC_RXCSR_H_WZC_BITS ((1 << USBC_BP_RXCSR_H_RX_STALL) | (1 << USBC_BP_RXCSR_H_ERROR) | (1 << USBC_BP_RXCSR_H_DATA_ERR) | (1 << USBC_BP_RXCSR_H_RX_PKT_READY))

/* Rx ep Control and Status Register for Device only */
#define USBC_BP_RXCSR_D_AUTO_CLEAR 15
#define USBC_BP_RXCSR_D_ISO 14
#define USBC_BP_RXCSR_D_DMA_REQ_EN 13
#define USBC_BP_RXCSR_D_DISABLE_NYET 12
#define USBC_BP_RXCSR_D_DMA_REQ_MODE 11

#define USBC_BP_RXCSR_D_INCOMPLETE 8
#define USBC_BP_RXCSR_D_CLEAR_DATA_TOGGLE 7
#define USBC_BP_RXCSR_D_SENT_STALL 6
#define USBC_BP_RXCSR_D_SEND_STALL 5
#define USBC_BP_RXCSR_D_FLUSH_FIFO 4
#define USBC_BP_RXCSR_D_DATA_ERROR 3
#define USBC_BP_RXCSR_D_OVERRUN 2
#define USBC_BP_RXCSR_D_FIFO_FULL 1
#define USBC_BP_RXCSR_D_RX_PKT_READY 0

/* Tx Type Register for host only */
#define USBC_BP_TXTYPE_SPEED 6 /*new*/
#define USBC_BP_TXTYPE_PROROCOL 4
#define USBC_BP_TXTYPE_TARGET_EP_NUM 0

/* Rx Type Register for host only */
#define USBC_BP_RXTYPE_SPEED 6 /*new*/
#define USBC_BP_RXTYPE_PROROCOL 4
#define USBC_BP_RXTYPE_TARGET_EP_NUM 0

/* Core Configueation */
#define USBC_BP_CONFIGDATA_MPRXE 7
#define USBC_BP_CONFIGDATA_MPTXE 6
#define USBC_BP_CONFIGDATA_BIGENDIAN 5
#define USBC_BP_CONFIGDATA_HBRXE 4
#define USBC_BP_CONFIGDATA_HBTXE 3
#define USBC_BP_CONFIGDATA_DYNFIFO_SIZING 2
#define USBC_BP_CONFIGDATA_SOFTCONE 1
#define USBC_BP_CONFIGDATA_UTMI_DATAWIDTH 0

/* OTG Device Control */
#define USBC_BP_DEVCTL_B_DEVICE 7
#define USBC_BP_DEVCTL_FS_DEV 6
#define USBC_BP_DEVCTL_LS_DEV 5

#define USBC_BP_DEVCTL_VBUS 3
#define USBC_BP_DEVCTL_HOST_MODE 2
#define USBC_BP_DEVCTL_HOST_REQ 1
#define USBC_BP_DEVCTL_SESSION 0

/* Tx EP FIFO size control */
#define USBC_BP_TXFIFOSZ_DPB 4
#define USBC_BP_TXFIFOSZ_SZ 0

/* Rx EP FIFO size control */
#define USBC_BP_RXFIFOSZ_DPB 4
#define USBC_BP_RXFIFOSZ_SZ 0

/* vendor0 */
#define USBC_BP_VEND0_DRQ_SEL 1
#define USBC_BP_VEND0_BUS_SEL 0

/* hub address */
#define USBC_BP_HADDR_MULTI_TT 7

/* Interface Status and Control */
#define USBC_BP_ISCR_VBUS_VALID_FROM_DATA 30
#define USBC_BP_ISCR_VBUS_VALID_FROM_VBUS 29
#define USBC_BP_ISCR_EXT_ID_STATUS 28
#define USBC_BP_ISCR_EXT_DM_STATUS 27
#define USBC_BP_ISCR_EXT_DP_STATUS 26
#define USBC_BP_ISCR_MERGED_VBUS_STATUS 25
#define USBC_BP_ISCR_MERGED_ID_STATUS 24

#define USBC_BP_ISCR_ID_PULLUP_EN 17
#define USBC_BP_ISCR_DPDM_PULLUP_EN 16
#define USBC_BP_ISCR_FORCE_ID 14
#define USBC_BP_ISCR_FORCE_VBUS_VALID 12
#define USBC_BP_ISCR_VBUS_VALID_SRC 10

#define USBC_BP_ISCR_HOSC_EN 7
#define USBC_BP_ISCR_VBUS_CHANGE_DETECT 6
#define USBC_BP_ISCR_ID_CHANGE_DETECT 5
#define USBC_BP_ISCR_DPDM_CHANGE_DETECT 4
#define USBC_BP_ISCR_IRQ_ENABLE 3
#define USBC_BP_ISCR_VBUS_CHANGE_DETECT_EN 2
#define USBC_BP_ISCR_ID_CHANGE_DETECT_EN 1
#define USBC_BP_ISCR_DPDM_CHANGE_DETECT_EN 0

/*-----------------------------------------------------------------------*/
/* Custom definitions */
/*-----------------------------------------------------------------------*/

/* USB resource description */
#define USBC_MAX_CTL_NUM 3
#define USBC_MAX_EP_NUM 6 /* Maximum supported EP number, ep0~5 */
#define USBC0_MAX_FIFO_SIZE (8 * 1024)
#define USBC_EP0_FIFOSIZE 64 /* This is non-configurable */

/* usb OTG mode */
#define USBC_OTG_HOST 0
#define USBC_OTG_DEVICE 1

/* usb device type */
#define USBC_DEVICE_HSDEV 0
#define USBC_DEVICE_FSDEV 1
#define USBC_DEVICE_LSDEV 2

/*  usb transfer type  */
#define USBC_TS_TYPE_IDLE 0
#define USBC_TS_TYPE_CTRL 1
#define USBC_TS_TYPE_ISO 2
#define USBC_TS_TYPE_INT 3
#define USBC_TS_TYPE_BULK 4

/*  usb transfer mode  */
#define USBC_TS_MODE_UNKOWN 0
#define USBC_TS_MODE_LS 1
#define USBC_TS_MODE_FS 2
#define USBC_TS_MODE_HS 3

/* usb Vbus status */
#define USBC_VBUS_STATUS_BELOW_SESSIONEND 0
#define USBC_VBUS_STATUS_ABOVE_SESSIONEND_BELOW_AVALID 1
#define USBC_VBUS_STATUS_ABOVE_AVALID_BELOW_VBUSVALID 2
#define USBC_VBUS_STATUS_ABOVE_VBUSVALID 3

/* usb io type */
#define USBC_IO_TYPE_PIO 0
#define USBC_IO_TYPE_DMA 1

/* usb ep type */
#define USBC_EP_TYPE_IDLE 0
#define USBC_EP_TYPE_EP0 1
#define USBC_EP_TYPE_TX 2
#define USBC_EP_TYPE_RX 3

/* usb id type */
#define USBC_ID_TYPE_DISABLE 0
#define USBC_ID_TYPE_HOST 1
#define USBC_ID_TYPE_DEVICE 2

/* usb vbus valid type */
#define USBC_VBUS_TYPE_DISABLE 0
#define USBC_VBUS_TYPE_LOW 1
#define USBC_VBUS_TYPE_HIGH 2

/* usb a valid source */
#define USBC_A_VALID_SOURCE_UTMI_AVALID 0
#define USBC_A_VALID_SOURCE_UTMI_VBUS 1

/* usb device switch */
#define USBC_DEVICE_SWITCH_OFF 0
#define USBC_DEVICE_SWITCH_ON 1

/* usb fifo config mode */
#define USBC_FIFO_MODE_4K 0
#define USBC_FIFO_MODE_8K 1

/* usb dma transfer direction */
#define USB_DMA_FROM_DRAM_TO_HOST 0
#define USB_DMA_FROM_HOST_TO_DRAM 1

/*
 **************************************************
 *  usb interrupt mask
 *
 **************************************************
 */

/* interrupt flags for ep0 and the Tx ep1~4 */
#define USBC_INTTx_FLAG_EP5 (1 << USBC_BP_INTTx_FLAG_EP5)
#define USBC_INTTx_FLAG_EP4 (1 << USBC_BP_INTTx_FLAG_EP4)
#define USBC_INTTx_FLAG_EP3 (1 << USBC_BP_INTTx_FLAG_EP3)
#define USBC_INTTx_FLAG_EP2 (1 << USBC_BP_INTTx_FLAG_EP2)
#define USBC_INTTx_FLAG_EP1 (1 << USBC_BP_INTTx_FLAG_EP1)
#define USBC_INTTx_FLAG_EP0 (1 << USBC_BP_INTTx_FLAG_EP0)

/* interrupt flags for Rx ep1~4 */
#define USBC_INTRx_FLAG_EP5 (1 << USBC_BP_INTRx_FLAG_EP5)
#define USBC_INTRx_FLAG_EP4 (1 << USBC_BP_INTRx_FLAG_EP4)
#define USBC_INTRx_FLAG_EP3 (1 << USBC_BP_INTRx_FLAG_EP3)
#define USBC_INTRx_FLAG_EP2 (1 << USBC_BP_INTRx_FLAG_EP2)
#define USBC_INTRx_FLAG_EP1 (1 << USBC_BP_INTRx_FLAG_EP1)

/* USB interrupt */
#define USBC_INTUSB_VBUS_ERROR (1 << USBC_BP_INTUSB_VBUS_ERROR)
#define USBC_INTUSB_SESSION_REQ (1 << USBC_BP_INTUSB_SESSION_REQ)
#define USBC_INTUSB_DISCONNECT (1 << USBC_BP_INTUSB_DISCONNECT)
#define USBC_INTUSB_CONNECT (1 << USBC_BP_INTUSB_CONNECT)
#define USBC_INTUSB_SOF (1 << USBC_BP_INTUSB_SOF)
#define USBC_INTUSB_RESET (1 << USBC_BP_INTUSB_RESET)
#define USBC_INTUSB_RESUME (1 << USBC_BP_INTUSB_RESUME)
#define USBC_INTUSB_SUSPEND (1 << USBC_BP_INTUSB_SUSPEND)

#define USBC_PHY_CTL_VBUSVLDEXT 5
#define USBC_PHY_CTL_SIDDQ 3

#endif /* __DRIVERS_USB_USB_REGS_H__ */
