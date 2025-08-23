/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __SUN8IW21_REG_CCU_H__
#define __SUN8IW21_REG_CCU_H__

#include <reg-ncat.h>

#define CCU_BASE SUNXI_CCU_BASE

#define CCU_PLL_CPU_CTRL_REG 0x0000		   /* PLL_CPU Control Register */
#define CCU_PLL_DDR_CTRL_REG 0x0010		   /* PLL_DDR Control Register */
#define CCU_PLL_PERI_CTRL_REG 0x0020	   /* PLL_PERI Control Register */
#define CCU_PLL_VIDEO_CTRL_REG 0x0040	   /* PLL_VIDEO Control Register */
#define CCU_PLL_CSI_CTRL_REG 0x0048		   /* PLL_CSI Control Register */
#define CCU_PLL_AUDIO_CTRL_REG 0x0078	   /* PLL_AUDIO Control Register */
#define CCU_PLL_NPU_CTRL_REG 0x0080		   /* PLL_NPU Control Register */
#define CCU_PLL_DDR_PAT0_CTRL_REG 0x0110   /* PLL_DDR Pattern0 Control Register */
#define CCU_PLL_DDR_PAT1_CTRL_REG 0x0114   /* PLL_DDR Pattern1 Control Register */
#define CCU_PLL_PERI_PAT0_CTRL_REG 0x0120  /* PLL_PERI Pattern0 Control Register */
#define CCU_PLL_PERI_PAT1_CTRL_REG 0x0124  /* PLL_PERI Pattern1 Control Register */
#define CCU_PLL_VIDEO_PAT0_CTRL_REG 0x0140 /* PLL_VIDEO Pattern0 Control Register */
#define CCU_PLL_VIDEO_PAT1_CTRL_REG 0x0144 /* PLL_VIDEO Pattern1 Control Register */
#define CCU_PLL_CSI_PAT0_CTRL_REG 0x0148   /* PLL_CSI Pattern0 Control Register */
#define CCU_PLL_CSI_PAT1_CTRL_REG 0x014C   /* PLL_CSI Pattern1 Control Register */
#define CCU_PLL_AUDIO_PAT0_CTRL_REG 0x0178 /* PLL_AUDIO Pattern0 Control Register */
#define CCU_PLL_AUDIO_PAT1_CTRL_REG 0x017C /* PLL_AUDIO Pattern1 Control Register */
#define CCU_PLL_NPU_PAT0_CTRL_REG 0x0180   /* PLL_NPU Pattern0 Control Register */
#define CCU_PLL_NPU_PAT1_CTRL_REG 0x0184   /* PLL_NPU Pattern1 Control Register */
#define CCU_PLL_CPU_BIAS_REG 0x0300		   /* PLL_CPU Bias Register */
#define CCU_PLL_DDR_BIAS_REG 0x0310		   /* PLL_DDR Bias Register */
#define CCU_PLL_PERI_BIAS_REG 0x0320	   /* PLL_PERI Bias Register */
#define CCU_PLL_VIDEO_BIAS_REG 0x0340	   /* PLL_VIDEO Bias Register */
#define CCU_PLL_CSI_BIAS_REG 0x0348		   /* PLL_CSI Bias Register */
#define CCU_PLL_AUDIO_BIAS_REG 0x0378	   /* PLL_AUDIO Bias Register */
#define CCU_PLL_NPU_BIAS_REG 0x0380		   /* PLL_NPU Bias Register */
#define CCU_PLL_CPU_TUN_REG 0x0400		   /* PLL_CPU Tuning Register */
#define CCU_CPU_CLK_REG 0x0500			   /* CPU Clock Register */
#define CCU_CPU_GATING_REG 0x0504		   /* CPU Gating Configuration Register */
#define CCU_AHB_CLK_REG 0x0510			   /* AHB Clock Register */
#define CCU_APB0_CLK_REG 0x0520			   /* APB0 Clock Register */
#define CCU_APB1_CLK_REG 0x0524			   /* APB1 Clock Register */
#define CCU_MBUS_CLK_REG 0x0540			   /* MBUS Clock Register */
#define CCU_DE_CLK_REG 0x0600			   /* DE Clock Register */
#define CCU_DE_BGR_REG 0x060C			   /* DE Bus Gating Reset Register */
#define CCU_G2D_CLK_REG 0x0630			   /* G2D Clock Register */
#define CCU_G2D_BGR_REG 0x063C			   /* G2D Bus Gating Reset Register */
#define CCU_CE_CLK_REG 0x0680			   /* CE Clock Register */
#define CCU_CE_BGR_REG 0x068C			   /* CE Bus Gating Reset Register */
#define CCU_VE_CLK_REG 0x0690			   /* VE Clock Register */
#define CCU_VE_BGR_REG 0x069C			   /* VE Bus Gating Reset Register */
#define CCU_NPU_CLK_REG 0x06E0			   /* NPU Clock Register */
#define CCU_NPU_BGR_REG 0x06EC			   /* NPU Bus Gating Reset Register */
#define CCU_DMA_BGR_REG 0x070C			   /* DMA Bus Gating Reset Register */
#define CCU_MSGBOX_BGR_REG 0x071C		   /* MSGBOX Bus Gating Reset Register */
#define CCU_SPINLOCK_BGR_REG 0x072C		   /* SPINLOCK Bus Gating Reset Register */
#define CCU_HSTIMER_BGR_REG 0x073C		   /* HSTIMER Bus Gating Reset Register */
#define CCU_AVS_CLK_REG 0x0740			   /* AVS Clock Register */
#define CCU_DBGSYS_BGR_REG 0x078C		   /* DBGSYS Bus Gating Reset Register */
#define CCU_PWM_BGR_REG 0x07AC			   /* PWM Bus Gating Reset Register */
#define CCU_IOMMU_BGR_REG 0x07BC		   /* IOMMU Bus Gating Reset Register */
#define CCU_DRAM_CLK_REG 0x0800			   /* DRAM Clock Register */
#define CCU_MBUS_MAT_CLK_GATING_REG 0x0804 /* MBUS Master Clock Gating Register */
#define CCU_DRAM_BGR_REG 0x080C			   /* DRAM Bus Gating Reset Register */
#define CCU_SMHC0_CLK_REG 0x0830		   /* SMHC0 Clock Register */
#define CCU_SMHC1_CLK_REG 0x0834		   /* SMHC1 Clock Register */
#define CCU_SMHC2_CLK_REG 0x0838		   /* SMHC2 Clock Register */
#define CCU_SMHC_BGR_REG 0x084C			   /* SMHC Bus Gating Reset Register */
#define CCU_UART_BGR_REG 0x090C			   /* UART Bus Gating Reset Register */
#define CCU_TWI_BGR_REG 0x091C			   /* TWI Bus Gating Reset Register */
#define CCU_SPI0_CLK_REG 0x0940			   /* SPI0 Clock Register */
#define CCU_SPI1_CLK_REG 0x0944			   /* SPI1 Clock Register */
#define CCU_SPI2_CLK_REG 0x0948			   /* SPI2 Clock Register */
#define CCU_SPI3_CLK_REG 0x094C			   /* SPI3 Clock Register */
#define CCU_SPI_BGR_REG 0x096C			   /* SPI Bus Gating Reset Register */
#define CCU_EMAC_25M_CLK_REG 0x0970		   /* EMAC_25M Clock Register */
#define CCU_EMAC_BGR_REG 0x097C			   /* EMAC Bus Gating Reset Register */
#define CCU_GPADC_BGR_REG 0x09EC		   /* GPADC Bus Gating Reset Register */
#define CCU_THS_BGR_REG 0x09FC			   /* THS Bus Gating Reset Register */
#define CCU_I2S1_CLK_REG 0x0A14			   /* I2S1 Clock Register */
#define CCU_I2S_BGR_REG 0x0A20			   /* I2S Bus Gating Reset Register */
#define CCU_DMIC_CLK_REG 0x0A40			   /* DMIC Clock Register */
#define CCU_DMIC_BGR_REG 0x0A4C			   /* DMIC Bus Gating Reset Register */
#define CCU_AUDIO_CODEC_DAC_CLK_REG 0x0A50 /* AUDIO_CODEC_DAC Clock Register */
#define CCU_AUDIO_CODEC_ADC_CLK_REG 0x0A54 /* AUDIO_CODEC_ADC Clock Register */
#define CCU_AUDIO_CODEC_BGR_REG 0x0A5C	   /* AUDIO_CODEC Bus Gating Reset Register */
#define CCU_USB0_CLK_REG 0x0A70			   /* USB0 Clock Register */
#define CCU_USB_BGR_REG 0x0A8C			   /* USB Bus Gating Reset Register */
#define CCU_DPSS_TOP_BGR_REG 0x0ABC		   /* DPSS_TOP Bus Gating Reset Register */
#define CCU_DSI_CLK_REG 0x0B24			   /* DSI Clock Register */
#define CCU_DSI_BGR_REG 0x0B4C			   /* DSI Bus Gating Reset Register */
#define CCU_TCONLCD_CLK_REG 0x0B60		   /* TCONLCD Clock Register */
#define CCU_TCONLCD_BGR_REG 0x0B7C		   /* TCONLCD Bus Gating Reset Register */
#define CCU_CSI_CLK_REG 0x0C04			   /* CSI Clock Register */
#define CCU_CSI_MASTER0_CLK_REG 0x0C08	   /* CSI Master0 Clock Register */
#define CCU_CSI_MASTER1_CLK_REG 0x0C0C	   /* CSI Master1 Clock Register */
#define CCU_CSI_MASTER2_CLK_REG 0x0C10	   /* CSI Master2 Clock Register */
#define CCU_CSI_BGR_REG 0x0C2C			   /* CSI Bus Gating Reset Register */
#define CCU_WIEGAND_BGR_REG 0x0C7C		   /* WIEGAND Bus Gating Reset Register */
#define CCU_PLL_PRE_DIV_REG 0x0E00		   /* PLL Pre Divider Register */
#define CCU_AHB_GATE_EN_REG 0x0E04		   /* AHB Gate Enable Register */
#define CCU_PERIPLL_GATE_EN_REG 0x0E08	   /* PERIPLL Gate Enable Register */
#define CCU_CLK24M_GATE_EN_REG 0x0E0C	   /* CLK24M Gate Enable Register */
#define CCU_CCMU_SEC_SWITCH_REG 0x0F00	   /* CCMU Security Switch Register */
#define CCU_GPADC_CLK_SEL_REG 0x0F04	   /* GPADC Clock Select Register */
#define CCU_FRE_DET_CTRL_REG 0x0F08		   /* Frequency Detect Control Register */
#define CCU_FRE_UP_LIM_REG 0x0F0C		   /* Frequency Up Limit Register */
#define CCU_FRE_DOWN_LIM_REG 0x0F10		   /* Frequency Down Limit Register */
#define CCU_CCMU_FAN_GATE_REG 0x0F30	   /* CCMU FANOUT CLOCK GATE Register */
#define CCU_CLK27M_FAN_REG 0x0F34		   /* CLK27M FANOUT Register */
#define CCU_CLK_FAN_REG 0x0F38			   /* CLK FANOUT Register */
#define CCU_CCMU_FAN_REG 0x0F3C			   /* CCMU FANOUT Register */

/* MMC clock bit field */
#define CCU_MMC_CTRL_M(x) ((x) -1)
#define CCU_MMC_CTRL_N(x) ((x) << 8)
#define CCU_MMC_CTRL_OSCM24 (0x0 << 24)
#define CCU_MMC_CTRL_PERI_400M (0x1 << 24)
#define CCU_MMC_CTRL_PERI_300M (0x2 << 24)
#define CCU_MMC_CTRL_PLL_PERIPH1X CCU_MMC_CTRL_PERI_400M
#define CCU_MMC_CTRL_PLL_PERIPH2X CCU_MMC_CTRL_PERI_300M
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

/* This file defines the register addresses and bit fields for
 * controlling the RISC-V subsystem in the CCU module. 
 */
#define CCU_RISCV_GATING_RST_REG (CCU_BASE + 0x0d04)// Register address for RISC-V gating reset control
#define CCU_RISCV_GATING_RST_FIELD (0x16aa << 16)	// Bit field value for RISC-V gating reset
#define CCU_RISCV_SYS_APB_SOFT_RSTN (0x1 << 2)		// Bit field value for RISC-V system APB soft reset
#define CCU_RISCV_SOFT_RSTN (0x1 << 1)				// Bit field value for RISC-V soft reset
#define CCU_RISCV_CLK_GATING (0x1 << 0)				// Bit field value for RISC-V clock gating

#define CCU_RISCV_CLK_REG (CCU_BASE + 0x0d00)// Register address for RISC-V clock control
#define CCU_RISCV_CLK_MASK (0x7 << 24)		 // Bit mask for RISC-V clock selection
#define CCU_RISCV_CLK_HOSC (0)				 // Bit field value for high-frequency oscillator clock
#define CCU_RISCV_CLK_32K (0x1 << 24)		 // Bit field value for 32 kHz clock
#define CCU_RISCV_CLK_16M (0x2 << 24)		 // Bit field value for 16 MHz clock
#define CCU_RISCV_CLK_PERI_600M (0x3 << 24)	 // Bit field value for 600 MHz peripheral clock
#define CCU_RISCV_CLK_PERI_480M (0x4 << 24)	 // Bit field value for 480 MHz peripheral clock
#define CCU_RISCV_CLK_CPUPLL (0x5 << 24)	 // Bit field value for CPU PLL clock

#define CCU_PLL_CPUX_TUNING_REG (0x1400)// Register address for CPU PLL tuning control

#define CCU_RISCV_CFG_BGR_REG (CCU_BASE + 0x0d0c)// Register address for RISC-V configuration BGR control
#define CCU_RISCV_CFG_RST (0x1 << 16)			 // Bit field value for RISC-V configuration reset
#define CCU_RISCV_CFG_GATING (0x1 << 0)			 // Bit field value for RISC-V configuration gating

#endif// __SUN8IW21_REG_CCU_H__
