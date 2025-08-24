/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2013-2025
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * 
 * original from bsp uboot defines
 */

#ifndef __SUN50IW9_REG_CCU_H__
#define __SUN50IW9_REG_CCU_H__

#include <reg-ncat.h>

#define CCU_BASE SUNXI_CCM_BASE

/* pll list */
#define CCU_PLL_CPUX_CTRL_REG (0x00)
#define CCU_PLL_DDR0_CTRL_REG (0x10)
#define CCU_PLL_DDR1_CTRL_REG (0x18)
#define CCU_PLL_PERI0_CTRL_REG (0x20)
#define CCU_PLL_PERI1_CTRL_REG (0x28)

#define CCU_PLL_HSIC_CTRL_REG (0x70)


/* cfg list */
#define CCU_CPUX_AXI_CFG_REG (0x500)
#define CCU_PSI_AHB1_AHB2_CFG_REG (0x510)
#define CCU_AHB3_CFG_GREG (0x51C)
#define CCU_APB1_CFG_GREG (0x520)
#define CCU_APB2_CFG_GREG (0x524)
#define CCU_MBUS_CFG_REG (0x540)

#define CCU_CE_CLK_REG (0x680)
#define CCU_CE_BGR_REG (0x68C)

#define CCU_VE_CLK_REG (0x690)
#define CCU_VE_BGR_REG (0x69C)

/*SYS*/
#define CCU_DMA_BGR_REG (0x70C)
#define CCU_AVS_CLK_REG (0x740)
#define CCU_AVS_BGR_REG (0x74C)

/*IOMMU*/
#define CCU_IOMMU_BGR_REG (0x7bc)
#define IOMMU_AUTO_GATING_REG (SUNXI_IOMMU_BASE + 0X40)

/* storage */
#define CCU_DRAM_CLK_REG (0x800)
#define CCU_MBUS_MAT_CLK_GATING_REG (0x804)
#define CCU_PLL_DDR_AUX_REG (0x808)
#define CCU_DRAM_BGR_REG (0x80C)

#define CCU_NAND_CLK_REG (0x810)
#define CCU_NAND_BGR_REG (0x82C)

#define CCU_SMHC0_CLK_REG (0x830)
#define CCU_SMHC1_CLK_REG (0x834)
#define CCU_SMHC2_CLK_REG (0x838)
#define CCU_SMHC_BGR_REG (0x84c)

/*normal interface*/
#define CCU_UART_BGR_REG (0x90C)
#define CCU_TWI_BGR_REG (0x91C)

#define CCU_SCR_BGR_REG (0x93C)

#define CCU_SPI0_CLK_REG (0x940)
#define CCU_SPI1_CLK_REG (0x944)
#define CCU_SPI_BGR_REG (0x96C)
#define CCU_USB0_CLK_REG (0xA70)
#define CCU_USB_BGR_REG (0xA8C)

/*DMA*/
#define DMA_GATING_BASE CCU_DMA_BGR_REG
#define DMA_GATING_PASS (1)
#define DMA_GATING_BIT (0)

/*CE*/
#define CE_CLK_SRC_MASK (0x1)
#define CE_CLK_SRC_SEL_BIT (24)
#define CE_CLK_SRC (0x01)

#define CE_CLK_DIV_RATION_N_BIT (8)
#define CE_CLK_DIV_RATION_N_MASK (0x3)
#define CE_CLK_DIV_RATION_N (0)

#define CE_CLK_DIV_RATION_M_BIT (0)
#define CE_CLK_DIV_RATION_M_MASK (0xF)
#define CE_CLK_DIV_RATION_M (3)

#define CE_SCLK_ONOFF_BIT (31)
#define CE_SCLK_ON (1)

#define CE_GATING_BASE CCU_CE_BGR_REG
#define CE_GATING_PASS (1)
#define CE_GATING_BIT (0)

#define CE_RST_REG_BASE CCU_CE_BGR_REG
#define CE_RST_BIT (16)
#define CE_DEASSERT (1)

/*gpadc gate and reset reg*/
#define CCU_GPADC_BGR_REG (0x09EC)

/* ehci */
#define BUS_CLK_GATING_REG 0x60
#define BUS_SOFTWARE_RESET_REG 0x2c0
#define USBPHY_CONFIG_REG 0xcc

#define USBEHCI0_RST_BIT 24
#define USBEHCI0_GATIING_BIT 24
#define USBPHY0_RST_BIT 0
#define USBPHY0_SCLK_GATING_BIT 8

#define USBEHCI1_RST_BIT 25
#define USBEHCI1_GATIING_BIT 25
#define USBPHY1_RST_BIT 1
#define USBPHY1_SCLK_GATING_BIT 9

/* MMC clock bit field */
#define CCU_MMC_CTRL_M(x) ((x) -1)
#define CCU_MMC_CTRL_N(x) ((x) << 8)
#define CCU_MMC_CTRL_OSCM24 (0x0 << 24)
#define CCU_MMC_CTRL_PLL6X1 (0x1 << 24)
#define CCU_MMC_CTRL_PLL6X2 (0x2 << 24)
#define CCU_MMC_CTRL_PLL_PERIPH1X CCU_MMC_CTRL_PLL6X1
#define CCU_MMC_CTRL_PLL_PERIPH2X CCU_MMC_CTRL_PLL6X2
#define CCU_MMC_CTRL_ENABLE (0x1 << 31)

/* if doesn't have these delays */
#define CCU_MMC_CTRL_OCLK_DLY(a) ((void) (a), 0)
#define CCU_MMC_CTRL_SCLK_DLY(a) ((void) (a), 0)

#define CCU_MMC_BGR_SMHC0_GATE (1 << 0)
#define CCU_MMC_BGR_SMHC1_GATE (1 << 1)
#define CCU_MMC_BGR_SMHC2_GATE (1 << 2)

#define CCU_MMC_BGR_SMHC0_RST (1 << 16)
#define CCU_MMC_BGR_SMHC1_RST (1 << 17)
#define CCU_MMC_BGR_SMHC2_RST (1 << 18)

#endif// __SUN50IW9_REG_CCU_H__
