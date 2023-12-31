/* SPDX-License-Identifier: Apache-2.0 */

#ifndef __SUN55IW3_REG_CCU_H__
#define __SUN55IW3_REG_CCU_H__

#include <reg-ncat.h>

#include "reg-cpu.h"

#define CCU_BASE SUNXI_CCMU_BASE

#define APB2_CLK_SRC_OSC24M (APB1_CLK_REG_CLK_SRC_SEL_HOSC << APB1_CLK_REG_CLK_SRC_SEL_OFFSET)
#define APB2_CLK_SRC_OSC32K (APB2_CLK_SRC_OSC32K << APB1_CLK_REG_CLK_SRC_SEL_OFFSET)
#define APB2_CLK_SRC_PSI (APB1_CLK_REG_CLK_SRC_SEL_CLK16M_RC << APB1_CLK_REG_CLK_SRC_SEL_OFFSET)
#define APB2_CLK_SRC_PLL6 (APB1_CLK_REG_CLK_SRC_SEL_PERI0_600M_BUS << APB1_CLK_REG_CLK_SRC_SEL_OFFSET)

#define APB2_CLK_RATE_N_1 (0x0 << 8)
#define APB2_CLK_RATE_N_2 (0x1 << 8)
#define APB2_CLK_RATE_N_4 (0x2 << 8)
#define APB2_CLK_RATE_N_8 (0x3 << 8)
#define APB2_CLK_RATE_N_MASK (3 << 8)
#define APB2_CLK_RATE_M(m) (((m) -1) << APB1_CLK_REG_FACTOR_M_OFFSET)
#define APB2_CLK_RATE_M_MASK (3 << APB1_CLK_REG_FACTOR_M_OFFSET)

/* MMC clock bit field */
#define CCU_MMC_CTRL_M(x) ((x) -1)
#define CCU_MMC_CTRL_N(x) ((x) << SMHC0_CLK_REG_FACTOR_N_OFFSET)
#define CCU_MMC_CTRL_OSCM24 (SMHC0_CLK_REG_CLK_SRC_SEL_HOSC << SMHC0_CLK_REG_CLK_SRC_SEL_OFFSET)
#define CCU_MMC_CTRL_PLL6X2 (SMHC0_CLK_REG_CLK_SRC_SEL_PERI0_400M << SMHC0_CLK_REG_CLK_SRC_SEL_OFFSET)
#define CCU_MMC_CTRL_PLL_PERIPH2X2 (SMHC0_CLK_REG_CLK_SRC_SEL_PERI0_300M << SMHC0_CLK_REG_CLK_SRC_SEL_OFFSET)
#define CCU_MMC_CTRL_ENABLE (SMHC0_CLK_REG_SMHC0_CLK_GATING_CLOCK_IS_ON << SMHC0_CLK_REG_SMHC0_CLK_GATING_OFFSET)
/* if doesn't have these delays */
#define CCU_MMC_CTRL_OCLK_DLY(a) ((void) (a), 0)
#define CCU_MMC_CTRL_SCLK_DLY(a) ((void) (a), 0)

#define CCU_MMC_BGR_SMHC0_GATE (1 << 0)
#define CCU_MMC_BGR_SMHC1_GATE (1 << 1)
#define CCU_MMC_BGR_SMHC2_GATE (1 << 2)

#define CCU_MMC_BGR_SMHC0_RST (1 << 16)
#define CCU_MMC_BGR_SMHC1_RST (1 << 17)
#define CCU_MMC_BGR_SMHC2_RST (1 << 18)

/* Module gate/reset shift*/
#define RESET_SHIFT (16)
#define GATING_SHIFT (0)

/* pll list */
#define CCU_PLL_CPU0_CTRL_REG (SUNXI_CPU_SYS_CFG_BASE + 0x817000)
#define CCU_PLL_CPU1_CTRL_REG (SUNXI_CPU_SYS_CFG_BASE + 0x817000 + 0x04)
#define CCU_PLL_CPU2_CTRL_REG (SUNXI_CPU_SYS_CFG_BASE + 0x817000 + 0x08)
#define CCU_PLL_CPU3_CTRL_REG (SUNXI_CPU_SYS_CFG_BASE + 0x817000 + 0x0c)
#define CCU_PLL_DDR0_CTRL_REG (PLL_DDR_CTRL_REG)
#define CCU_PLL_DDR1_CTRL_REG (0x18)
#define CCU_PLL_PERI0_CTRL_REG (PLL_PERI0_CTRL_REG)
#define CCU_PLL_PERI1_CTRL_REG (PLL_PERI1_CTRL_REG)
#define CCU_PLL_GPU_CTRL_REG (PLL_GPU_CTRL_REG)
#define CCU_PLL_VIDE00_CTRL_REG (PLL_VIDEO0_CTRL_REG)
#define CCU_PLL_VIDE01_CTRL_REG (PLL_VIDEO1_CTRL_REG)
#define CCU_PLL_VIDE02_CTRL_REG (PLL_VIDEO2_CTRL_REG)
#define CCU_PLL_VIDE03_CTRL_REG (PLL_VIDEO3_CTRL_REG)
#define CCU_PLL_VE_CTRL_REG (PLL_VE_CTRL_REG)
#define CCU_PLL_CPUA_CLK_REG (SUNXI_CPU_SYS_CFG_BASE + 0x817000 + 0x60)
#define CCU_PLL_CPUB_CLK_REG (SUNXI_CPU_SYS_CFG_BASE + 0x817000 + 0x64)
#define CCU_PLL_CPU_CLK_REG (SUNXI_CPU_SYS_CFG_BASE + 0x817000 + 0x68)
#define CCU_PLL_DSU_CLK_REG (SUNXI_CPU_SYS_CFG_BASE + 0x817000 + 0x6C)
#define CCU_PLL_AUDIO_CTRL_REG (PLL_AUDIO_CTRL_REG)
#define CCU_PLL_HSIC_CTRL_REG (0x70)

/* cfg list */
#define CCU_CPUX_AXI_CFG_REG (CPU_CLK_REG)
#define CCU_AHB0_CFG_REG (0x510)
#define CCU_APB0_CFG_REG (0x520)
#define CCU_APB1_CFG_REG (0x524)
#define CCU_MBUS_CFG_REG (0x540)

#define CCU_CE_CLK_REG (0x680)
#define CCU_CE_BGR_REG (0x68C)

#define CCU_VE_CLK_REG (0x690)
#define CCU_VE_BGR_REG (0x69C)

/*SYS*/
#define CCU_DMA_BGR_REG (0x70C)
#define CCU_AVS_CLK_REG (0x750)
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
#define CE_CLK_SRC_MASK (0x7)
#define CE_CLK_SRC_SEL_BIT (CE_CLK_REG_CLK_SRC_SEL_OFFSET)
#define CE_CLK_SRC (CE_CLK_REG_CLK_SRC_SEL_PERI0_400M)

#define CE_CLK_DIV_RATION_N_BIT (0)
#define CE_CLK_DIV_RATION_N_MASK (0x0)
#define CE_CLK_DIV_RATION_N (0)

#define CE_CLK_DIV_RATION_M_BIT (CE_CLK_REG_FACTOR_M_OFFSET)
#define CE_CLK_DIV_RATION_M_MASK (CE_CLK_REG_FACTOR_M_CLEAR_MASK)
#define CE_CLK_DIV_RATION_M (0)

#define CE_SCLK_ONOFF_BIT (31)
#define CE_SCLK_ON (1)

#define CE_GATING_BASE CCU_CE_BGR_REG
#define CE_GATING_PASS (1)
#define CE_GATING_BIT (0)

#define CE_RST_REG_BASE CCU_CE_BGR_REG

#define CE_SYS_RST_BIT (CE_BGR_REG_CE_SYS_RST_OFFSET)
#define CE_RST_BIT (CE_BGR_REG_CE_RST_OFFSET)
#define CE_DEASSERT (CE_BGR_REG_CE_SYS_RST_DE_ASSERT)
#define CE_SYS_GATING_BIT (CE_BGR_REG_CE_SYS_GATING_OFFSET)

/*gpadc gate and reset reg*/
#define CCU_GPADC_BGR_REG (0x09EC)
/*gpadc gate and reset reg*/
#define CCU_GPADC_CLK_REG (0x09E0)
/*lpadc gate and reset reg*/
#define CCU_LRADC_BGR_REG (0x0A9C)

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

/* SPIF clock bit field */
#define CCM_SPIF_CTRL_M(x) ((x) -1)
#define CCM_SPIF_CTRL_N(x) ((x) << 8)
#define CCM_SPIF_CTRL_HOSC (0x0 << 24)
#define CCM_SPIF_CTRL_PERI400M (0x1 << 24)
#define CCM_SPIF_CTRL_PERI300M (0x2 << 24)
#define CCM_SPIF_CTRL_ENABLE (0x1 << 31)
#define GET_SPIF_CLK_SOURECS(x) (x == CCM_SPIF_CTRL_PERI400M ? 400000000 : 300000000)
#define CCM_SPIF_CTRL_PERI CCM_SPIF_CTRL_PERI400M
#define SPIF_RESET_SHIFT (19)
#define SPIF_GATING_SHIFT (3)

/*E906*/
#define RISCV_PUBSRAM_CFG_REG (SUNXI_DSP_PRCM_BASE + 0x0114)
#define RISCV_PUBSRAM_RST (0x1 << 16)
#define RISCV_PUBSRAM_GATING (0x1 << 0)

#define RISCV_CLK_REG (SUNXI_DSP_PRCM_BASE + 0x0120)
#define RISCV_CLK_GATING (0x1 << 31)

#define RISCV_CFG_BGR_REG (SUNXI_DSP_PRCM_BASE + 0x0124)
#define RISCV_CORE_RST (0x1 << 18)
#define RISCV_APB_DB_RST (0x1 << 17)
#define RISCV_CFG_RST (0x1 << 16)
#define RISCV_CFG_GATING (0x1 << 0)

#define RISCV_CFG_BASE (0x07130000)
#define RISCV_STA_ADD_REG (RISCV_CFG_BASE + 0x0204)

#endif// __SUN55IW3_REG_CCU_H__
