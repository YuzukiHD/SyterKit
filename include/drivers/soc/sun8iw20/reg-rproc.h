/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __REG_RPROC_H__
#define __REG_RPROC_H__

/*
 * DSP CFG Register
 */
#define DSP0_CFG_BASE (0x01700000)

/* DSP default reset vector address */
#define DSP_DEFAULT_RST_VEC (0x100000)

/*
 * DSP Register define
 */
#define DSP_ALT_RESET_VEC_REG (0x0000) /* DSP Reset Control Register */
#define DSP_CTRL_REG0 (0x0004)		   /* DSP Control Register0 */
#define DSP_PRID_REG (0x000c)		   /* DSP PRID Register */
#define DSP_STAT_REG (0x0010)		   /* DSP STAT Register */
#define DSP_BIST_CTRL_REG (0x0014)	   /* DSP BIST CTRL Register */
#define DSP_JTRST_REG (0x001c)		   /* DSP JTAG CONFIG RESET Register */
#define DSP_VER_REG (0x0020)		   /* DSP Version Register */

/*
 * DSP Control Register0
 */
#define BIT_RUN_STALL (0)
#define BIT_START_VEC_SEL (1)
#define BIT_DSP_CLKEN (2)

/*
 * DSP PRID Register
 */
#define PRID_MASK (0xff << 0)

/*
 * DSP STAT Register
 */
#define BIT_PFAULT_INFO_VALID (0)
#define BIT_PFAULT_ERROR (1)
#define BIT_DOUBLE_EXCE_ERROR (2)
#define BIT_XOCD_MODE (3)
#define BIT_DEBUG_MODE (4)
#define BIT_PWAIT_MODE (5)
#define BIT_IRAM0_LOAD_STORE (6)

/*
 * BIST Control Register
 */
#define BIT_BIST_EN (0)
#define BIST_WDATA_PAT_MASK (0x7 << 1)
#define BIT_BIST_ADDR_MODE_SEL (4)
#define BIST_REG_SEL_MASK (0x7 << 5)
#define BIT_BIST_BUSY (8)
#define BIT_BIST_STOP (9)
#define BIST_ERR_CYC_MASK (0x3 << 10)
#define BIST_ERR_PAT_MASK (0x7 << 12)
#define BIT_BIST_ERR_STA (15)
#define BIST_SELECT_MASK (0xf << 16)

/*
 * DSP Version Register
 */
#define SMALL_VER_MASK (0x1f << 0)
#define LARGE_VER_MASK (0x1f << 16)

/*
 * DSP SRAM Remap Register
 */
#define BIT_SRAM_REMAP_ENABLE (0)
#define SRAMC_SRAM_REMAP_REG (0x8)

/*
 * RISC-V CFG Register
 */
#define RISCV_CFG_BASE (0x06010000)// Base address for RISC-V configuration

/*
 * RISC-V start address
 */
#define RISCV_STA_ADD_REG (RISCV_CFG_BASE + 0x0204)
#define RISCV_STA_ADD_L_REG (RISCV_CFG_BASE + 0x0004)
#define RISCV_STA_ADD_H_REG (RISCV_CFG_BASE + 0x0008)

#endif// __REG_RPROC_H__