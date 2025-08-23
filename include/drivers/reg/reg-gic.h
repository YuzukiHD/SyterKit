/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __REG_GIC_H__
#define __REG_GIC_H__

#include <reg-ncat.h>

#ifdef __cplusplus
extern "C" {
#endif// __cplusplus

#define GIC_DIST_BASE (SUNXI_GIC_BASE + 0x1000)
#define GIC_CPUIF_BASE (SUNXI_GIC_BASE + 0x2000)

#define GIC_CPU_IF_CTRL (GIC_CPUIF_BASE + 0x000)	 // 0x8000
#define GIC_INT_PRIO_MASK (GIC_CPUIF_BASE + 0x004)	 // 0x8004
#define GIC_BINARY_POINT (GIC_CPUIF_BASE + 0x008)	 // 0x8008
#define GIC_INT_ACK_REG (GIC_CPUIF_BASE + 0x00c)	 // 0x800c
#define GIC_END_INT_REG (GIC_CPUIF_BASE + 0x010)	 // 0x8010
#define GIC_RUNNING_PRIO (GIC_CPUIF_BASE + 0x014)	 // 0x8014
#define GIC_HIGHEST_PENDINT (GIC_CPUIF_BASE + 0x018) // 0x8018
#define GIC_DEACT_INT_REG (GIC_CPUIF_BASE + 0x1000)	 // 0x1000
#define GIC_AIAR_REG (GIC_CPUIF_BASE + 0x020)		 // 0x8020
#define GIC_AEOI_REG (GIC_CPUIF_BASE + 0x024)		 // 0x8024
#define GIC_AHIGHEST_PENDINT (GIC_CPUIF_BASE + 0x028)// 0x8028
#define GIC_IRQ_MOD_CFG(_n) (GIC_DIST_BASE + 0xc00 + 4 * (_n))

#define GIC_DIST_CON (GIC_DIST_BASE + 0x0000)
#define GIC_CON_TYPE (GIC_DIST_BASE + 0x0004)
#define GIC_CON_IIDR (GIC_DIST_BASE + 0x0008)

#define GIC_CON_IGRP(n) (GIC_DIST_BASE + 0x0080 + (n) *4)
#define GIC_SET_EN(_n) (GIC_DIST_BASE + 0x100 + 4 * (_n))
#define GIC_CLR_EN(_n) (GIC_DIST_BASE + 0x180 + 4 * (_n))
#define GIC_PEND_SET(_n) (GIC_DIST_BASE + 0x200 + 4 * (_n))
#define GIC_PEND_CLR(_n) (GIC_DIST_BASE + 0x280 + 4 * (_n))
#define GIC_ACT_SET(_n) (GIC_DIST_BASE + 0x300 + 4 * (_n))
#define GIC_ACT_CLR(_n) (GIC_DIST_BASE + 0x380 + 4 * (_n))
#define GIC_SGI_PRIO(_n) (GIC_DIST_BASE + 0x400 + 4 * (_n))
#define GIC_PPI_PRIO(_n) (GIC_DIST_BASE + 0x410 + 4 * (_n))
#define GIC_SPI_PRIO(_n) (GIC_DIST_BASE + 0x420 + 4 * (_n))
#define GIC_SPI_PROC_TARG(_n) (GIC_DIST_BASE + 0x820 + 4 * (_n))

/* software generated interrupt */
#define GIC_SRC_SGI(_n) (_n)
/* private peripheral interrupt */
#define GIC_SRC_PPI(_n) (16 + (_n))
/* external peripheral interrupt */
#define GIC_SRC_SPI(_n) (32 + (_n))

#ifdef __cplusplus
}
#endif// __cplusplus

#endif// __REG_GIC_H__