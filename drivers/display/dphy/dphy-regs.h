/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */
/*
 * Combo D-PHY (MIPI D-PHY TX + LVDS) register map, sun252iw2.
 */
#ifndef __DISPLAY_DPHY_REGS_H__
#define __DISPLAY_DPHY_REGS_H__

#include <stdint.h>

#define DISP_BIT(n)		   (1U << (n))
#define DISP_GENMASK(h, l)	   ((0xffffffffU >> (31 - (h))) & (0xffffffffU << (l)))
#define DISP_FIELD_PREP(mask, val) (((uint32_t)(val) << __builtin_ctz(mask)) & (mask))
#define DISP_FIELD_GET(mask, reg)  (((reg) & (mask)) >> __builtin_ctz(mask))

#define DPHY_GCTL	    0x000
#define DPHY_GCTL_LANE_NUM  DISP_GENMASK(5, 4)
#define DPHY_GCTL_MODULE_EN DISP_BIT(0)

#define DPHY_TX_CTL	      0x004
#define DPHY_TX_HSTX_CLK_CONT DISP_BIT(28)

#define DPHY_TX_TIME0	    0x010
#define DPHY_TX_HS_TRAIL    DISP_GENMASK(31, 24)
#define DPHY_TX_HS_PRE	    DISP_GENMASK(23, 16)
#define DPHY_TX_DTERM	    DISP_GENMASK(15, 8)
#define DPHY_TX_LPX	    DISP_GENMASK(7, 0)
#define DPHY_TX_TIME1	    0x014
#define DPHY_TX_CK_POST	    DISP_GENMASK(31, 24)
#define DPHY_TX_CK_PRE	    DISP_GENMASK(23, 16)
#define DPHY_TX_CK_ZERO	    DISP_GENMASK(15, 8)
#define DPHY_TX_CK_PREP	    DISP_GENMASK(7, 0)
#define DPHY_TX_TIME2	    0x018
#define DPHY_TX_HS_DLY_MODE DISP_BIT(28)
#define DPHY_TX_HS_DLY	    DISP_GENMASK(23, 8)
#define DPHY_TX_CK_TRAIL    DISP_GENMASK(7, 0)
#define DPHY_TX_TIME3	    0x01c
#define DPHY_TX_ULPS_EXIT   DISP_GENMASK(19, 0)
#define DPHY_TX_TIME4	    0x020
#define DPHY_TX_HSTX_ANA1   DISP_GENMASK(15, 8)
#define DPHY_TX_HSTX_ANA0   DISP_GENMASK(7, 0)

#define DPHY_ANA0	    0x04c
#define DPHY_ANA0_PWS	    DISP_BIT(31)
#define DPHY_ANA0_DMPC	    DISP_BIT(28)
#define DPHY_ANA0_DMPD	    DISP_GENMASK(27, 24)
#define DPHY_ANA0_SLV	    DISP_GENMASK(14, 12)
#define DPHY_ANA0_DEN	    DISP_GENMASK(11, 8)
#define DPHY_ANA0_LPTX_SETC DISP_GENMASK(6, 4)
#define DPHY_ANA0_LPTX_SETR DISP_GENMASK(2, 0)

#define DPHY_ANA1	  0x050
#define DPHY_ANA1_VTTMODE DISP_BIT(31)
#define DPHY_ANA1_CSMPS	  DISP_GENMASK(29, 28)
#define DPHY_ANA1_SVTT	  DISP_GENMASK(27, 24)

#define DPHY_ANA2	    0x054
#define DPHY_ANA2_ENP2S_CPU DISP_GENMASK(27, 24)
#define DPHY_ANA2_ENCK_CPU  DISP_BIT(4)
#define DPHY_ANA2_ENIB	    DISP_BIT(1)

#define DPHY_ANA3	 0x058
#define DPHY_ANA3_ENVTTD DISP_GENMASK(31, 28)
#define DPHY_ANA3_ENVTTC DISP_BIT(27)
#define DPHY_ANA3_ENDIV	 DISP_BIT(26)
#define DPHY_ANA3_ENLDOC DISP_BIT(25)
#define DPHY_ANA3_ENLDOD DISP_BIT(24)
#define DPHY_ANA3_ENLDOR DISP_BIT(18)

#define DPHY_ANA4	       0x05c
#define DPHY_ANA4_EN_MIPI      DISP_BIT(31)
#define DPHY_ANA4_IB	       DISP_GENMASK(26, 24)
#define DPHY_ANA4_VRES_SET     DISP_GENMASK(22, 20)
#define DPHY_ANA4_VTT_SET      DISP_GENMASK(18, 16)
#define DPHY_ANA4_VLPTX_SET    DISP_GENMASK(14, 12)
#define DPHY_ANA4_VLV_SET      DISP_GENMASK(10, 8)
#define DPHY_ANA4_EN_RESCAL    DISP_BIT(7)
#define DPHY_ANA4_EN_SOFT_RCAL DISP_BIT(5)
#define DPHY_ANA4_SOFT_RCAL    DISP_GENMASK(4, 0)

/* PLL: clk_hs = 24MHz * N / (P+1) / (M0+1) / (M1+1)
 *      clk_ls = 24MHz * N / (P+1) / (DIV0+1) / (DIV1+1) */
#define DPHY_PLL0	     0x104
#define DPHY_PLL0_REG_UPDATE DISP_BIT(31)
#define DPHY_PLL0_LS_DIV0    DISP_GENMASK(29, 28)
#define DPHY_PLL0_LS_DIV1    DISP_GENMASK(27, 24)
#define DPHY_PLL0_CP36_EN    DISP_BIT(23)
#define DPHY_PLL0_LDO_EN     DISP_BIT(22)
#define DPHY_PLL0_EN_LVS     DISP_BIT(21)
#define DPHY_PLL0_PLL_EN     DISP_BIT(20)
#define DPHY_PLL0_P	     DISP_GENMASK(19, 16)
#define DPHY_PLL0_N	     DISP_GENMASK(15, 8)
#define DPHY_PLL0_M0	     DISP_GENMASK(5, 4)
#define DPHY_PLL0_M1	     DISP_GENMASK(3, 0)
#define DPHY_PLL1	     0x108
#define DPHY_PLL1_HS_GATING  DISP_BIT(22)
#define DPHY_PLL1_LS_GATING  DISP_BIT(21)
#define DPHY_PLL1_LOCKDET_EN DISP_BIT(12)
#define DPHY_PLL2	     0x10c
#define DPHY_PLL2_SDM_EN     DISP_BIT(31)
#define DPHY_PLL2_FF_EN	     DISP_BIT(30)
#define DPHY_PLL2_SS_EN	     DISP_BIT(29)

#define COMBO_PHY_REG0	      0x110
#define COMBO_PHY_EN_MIPI     DISP_BIT(3)
#define COMBO_PHY_EN_LVDS     DISP_BIT(2)
#define COMBO_PHY_EN_LDO      DISP_BIT(1)
#define COMBO_PHY_EN_CP	      DISP_BIT(0)
#define COMBO_PHY_REG1	      0x114
#define COMBO_PHY_VREF1P6     DISP_GENMASK(6, 4)
#define COMBO_PHY_VREF0P8     DISP_GENMASK(2, 0)
#define COMBO_PHY_REG2	      0x118
#define COMBO_PHY_HS_STOP_DLY DISP_GENMASK(7, 0)

#endif /* __DISPLAY_DPHY_REGS_H__ */
