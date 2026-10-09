/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * TCON LCD field layout. Register offsets are not here: they live in the
 * per-SoC struct sunxi_tcon_regs so that another generation can move them.
 */
#ifndef __TCON_REGS_H__
#define __TCON_REGS_H__

#include <stdint.h>

#define TBIT(n)	    (1U << (n))
#define TMASK(h, l) (((0xffffffffU) >> (31 - (h))) & ~((1U << (l)) - 1U))
#define TPREP(m, v) ((((uint32_t)(v)) << __builtin_ctz(m)) & (m))
#define TGET(m, r)  (((r) & (m)) >> __builtin_ctz(m))

#define TCON_GCTL_EN	     TBIT(31)
#define TCON_GCTL_GAMMA_EN   TBIT(30)
#define TCON_GCTL_PAD_SEL    TBIT(1)
#define TCON_GCTL_IO_MAP_SEL TBIT(0)

#define TCON_GINT0_EN(irq)   TBIT(16 + (irq))
#define TCON_GINT0_FLAG(irq) TBIT(irq)
#define TCON_GINT0_FLAGS     TMASK(15, 0)

#define TCON0_FRM_EN	 TBIT(31)
#define TCON0_FRM_MODE_R TBIT(6)
#define TCON0_FRM_MODE_G TBIT(5)
#define TCON0_FRM_MODE_B TBIT(4)

#define TCON0_CTL_EN	      TBIT(31)
#define TCON0_CTL_IF	      TMASK(25, 24)
#define TCON0_IF_HV	      0
#define TCON0_IF_CPU	      1
#define TCON0_CTL_RB_SWAP     TBIT(23)
#define TCON0_CTL_RGB_SWAP    TMASK(18, 16)
#define TCON0_CTL_START_DELAY TMASK(8, 4)
#define TCON0_CTL_SRC_SEL     TMASK(2, 0)
#define TCON0_SRC_DE	      0
#define TCON0_SRC_COLORBAR    1
#define TCON0_SRC_GRAYSCALE   2
#define TCON0_SRC_BLACK_WHITE 3
#define TCON0_SRC_BLACK	      4
#define TCON0_SRC_WHITE	      5
#define TCON0_SRC_GRID	      7

#define TCON0_DCLK_EN  TMASK(31, 28)
#define TCON0_DCLK_DIV TMASK(6, 0)

#define TCON0_BASIC0_X	  TMASK(27, 16)
#define TCON0_BASIC0_Y	  TMASK(11, 0)
#define TCON0_BASIC1_HT	  TMASK(28, 16)
#define TCON0_BASIC1_HBP  TMASK(11, 0)
#define TCON0_BASIC2_VT	  TMASK(28, 16)
#define TCON0_BASIC2_VBP  TMASK(11, 0)
#define TCON0_BASIC3_HSPW TMASK(25, 16)
#define TCON0_BASIC3_VSPW TMASK(9, 0)

#define TCON0_HV_MODE	   TMASK(31, 28)
#define TCON0_HV_SRGB_SEQ  TMASK(27, 24)
#define TCON0_HV_SYUV_SEQ  TMASK(23, 22)
#define TCON0_HV_SYUV_FDLY TMASK(21, 20)

#define TCON0_CPU_MODE	      TMASK(31, 28)
#define TCON0_CPU_MODE_DSI    1
#define TCON0_CPU_DA	      TBIT(26)
#define TCON0_CPU_FLUSH	      TBIT(16)
#define TCON0_CPU_TRI_FIFO_EN TBIT(2)
#define TCON0_CPU_TRI_START   TBIT(1)
#define TCON0_CPU_TRI_EN      TBIT(0)

#define TCON0_LVDS_EN	    TBIT(31)
#define TCON0_LVDS_LINK	    TBIT(30)
#define TCON0_LVDS_MODE	    TBIT(27)
#define TCON0_LVDS_BITWIDTH TBIT(26)
#define TCON0_LVDS_CLK_SEL  TBIT(20)

#define TCON0_IO_DCLK_SEL	TMASK(30, 28)
#define TCON0_IO_DE_INV		TBIT(27)
#define TCON0_IO_CLK_INV	TBIT(26)
#define TCON0_IO_HSYNC_POSITIVE TBIT(25)
#define TCON0_IO_VSYNC_POSITIVE TBIT(24)

#define TCON_ECC_FIFO_SETTING TMASK(7, 0)
#define TCON_DEBUG_UNDERFLOW  TBIT(31)
#define TCON_DEBUG_LINE	      TMASK(27, 16)

#define TCON0_TRI0_BLOCK_SPACE	   TMASK(27, 16)
#define TCON0_TRI0_BLOCK_SIZE	   TMASK(11, 0)
#define TCON0_TRI1_BLOCK_NUM	   TMASK(15, 0)
#define TCON0_TRI2_START_DELAY	   TMASK(31, 16)
#define TCON0_TRI2_TRANS_START_SET TMASK(12, 0)
#define TCON0_TRI3_INT_MODE	   TMASK(29, 28)
#define TCON0_TRI3_COUNTER_N	   TMASK(23, 8)
#define TCON0_TRI3_COUNTER_M	   TMASK(7, 0)

#define TCON_SAFE_PERIOD_FIFO_NUM TMASK(28, 16)
#define TCON_SAFE_PERIOD_MODE	  TMASK(1, 0)

#define TCON0_LVDS_ANA_EN_MB   TBIT(31)
#define TCON0_LVDS_ANA_SRC_SEL TBIT(30)
#define TCON0_LVDS_ANA_EN_LVDS TBIT(29)
#define TCON0_LVDS_ANA_EN_24M  TBIT(28)
#define TCON0_LVDS_ANA_EN_DRVC TBIT(24)
#define TCON0_LVDS_ANA_EN_DRVD TMASK(23, 20)
#define TCON0_LVDS_ANA_C       TMASK(19, 17)
#define TCON0_LVDS_ANA_R       TMASK(10, 8)

#define TCON_SYNC_DSI_NUM TBIT(8)

/* TCON top */
#define TCON_TOP_TV0_OUT	TBIT(8)
#define TCON_TOP_TV1_OUT	TBIT(12)
#define TCON_TOP_DSI_SRC_SEL(n) TBIT((n) * 4)
#define TCON_TOP_LCD_CLK_SRC(n) TBIT(n)
#define TCON_TOP_PHY_CLK_SRC(n) TBIT(4 + (n))
#define TCON_TOP_DSI_CLK_GATE	TBIT(16)

#endif
