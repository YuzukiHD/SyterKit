/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file spif-regs.h
 * @brief SPI flash (SPIF) controller register map and transfer bit fields.
 *
 * Describes the SPIF controller registers, global control bits, DMA
 * descriptor fields and the opcodes recognized by the SPIF hardware.
 */

#ifndef __DRIVERS_SPIF_REGS_H__
#define __DRIVERS_SPIF_REGS_H__

#include <io.h>

/* SPIF controller registers. */
#define SPIF_VER_REG	 0x00U
#define SPIF_GC_REG	 0x04U
#define SPIF_GCA_REG	 0x08U
#define SPIF_TC_REG	 0x0cU
#define SPIF_INT_STA_REG 0x18U
#define SPIF_CSD_REG	 0x1cU
#define SPIF_PHC_REG	 0x20U
#define SPIF_TCF_REG	 0x24U
#define SPIF_TCS_REG	 0x28U
#define SPIF_TNM_REG	 0x2cU
#define SPIF_DMA_CTL_REG 0x40U
#define SPIF_DSC_REG	 0x44U

/* Global control and timing bits. */
#define SPIF_GC_CFG_MODE   BIT(0)
#define SPIF_GC_DMA_MODE   1U
#define SPIF_GC_CPU_MODE   0U
#define SPIF_GC_PMODE_EN   BIT(3)
#define SPIF_GC_NMODE_EN   BIT(2)
#define SPIF_GC_CPHA	   BIT(4)
#define SPIF_GC_CPOL	   BIT(5)
#define SPIF_GC_SS_MASK	   (3U << 6)
#define SPIF_GC_CS_POL	   BIT(8)
#define SPIF_GC_DTR_EN	   BIT(16)
#define SPIF_GC_RX_CFG_FBS BIT(17)
#define SPIF_GC_TX_CFG_FBS BIT(18)
#define SPIF_GC_HOLD_EN	   BIT(13)
#define SPIF_GC_WP_EN	   BIT(15)

#define SPIF_GCA_RF_SRST   BIT(0)
#define SPIF_GCA_WF_SRST   BIT(1)
#define SPIF_GCA_SOFT_SRST BIT(3)
#define SPIF_GCA_DMA_END   BIT(4)

#define SPIF_TC_ANALOG_DL_SW_RX_EN BIT(6)
#define SPIF_TC_DIGITAL_ANALOG_EN  BIT(20)
#define SPIF_TC_DIGITAL_DELAY_MASK (7U << 16)
#define SPIF_TC_ANALOG_DELAY_MASK  0x3fU
#define SPIF_TC_CLK_SCKOUT_SRC_SEL BIT(26)

/* Transfer phase and DMA control bits. */
#define SPIF_TRANS_RX_EN    BIT(8)
#define SPIF_TRANS_TX_EN    BIT(12)
#define SPIF_TRANS_DUMMY_EN BIT(16)
#define SPIF_TRANS_MODE_EN  BIT(20)
#define SPIF_TRANS_ADDR_EN  BIT(24)
#define SPIF_TRANS_CMD_EN   BIT(28)

#define SPIF_DMA_START		 BIT(0)
#define SPIF_DMA_DESCRIPTOR_LEN	 (32U << 4)
#define SPIF_DMA_FINISH_FLAG	 BIT(0)
#define SPIF_DMA_RW_PROCESS	 BIT(1)
#define SPIF_DMA_HBURST_MASK	 (7U << 4)
#define SPIF_DMA_HBURST_INCR4	 (3U << 4)
#define SPIF_DMA_HBURST_INCR16	 (7U << 4)
#define SPIF_DMA_BLOCK_LEN_MASK	 (0xffU << 24)
#define SPIF_DMA_BLOCK_LEN_64B	 (3U << 24)
#define SPIF_DMA_DATA_LEN_V0	 0xffffU
#define SPIF_DMA_DATA_LEN_V1	 0x1ffffU
#define SPIF_DMA_TRANS_NUM	 0xffffU
#define SPIF_DMA_TRANS_NUM_16BIT BIT(31)
#define SPIF_DMA_NORMAL_DESC	 BIT(28)
#define SPIF_DMA_DONE_INT	 BIT(24)

#define SPIF_CSD_DEFAULT    ((5U << 16) | (6U << 8) | 6U)
#define SPIF_TIMEOUT	    0x1000000U
#define SPIF_MAX_TRANS_SIZE 65536U
#define SPIF_MAX_TRANS_V0   4096U
#define SPIF_MAX_TRANS_V1   65536U
#define SPIF_MIN_TRANS_NUM  8U

/* Descriptor dword 6 and dword 7 fields. */
#define SPIF_DATA_TRANS_POS    0U
#define SPIF_MODE_TRANS_POS_V0 4U
#define SPIF_ADDR_TRANS_POS_V0 8U
#define SPIF_CMD_TRANS_POS_V0  12U
#define SPIF_MODE_TRANS_POS_V1 2U
#define SPIF_ADDR_TRANS_POS_V1 4U
#define SPIF_CMD_TRANS_POS_V1  6U
#define SPIF_MODE_OPCODE_POS   16U
#define SPIF_CMD_OPCODE_POS    24U

#define SPIF_ADDR_SIZE_24BIT	(0U << 24)
#define SPIF_ADDR_SIZE_32BIT	(1U << 24)
#define SPIF_ADDR_SIZE_MASK	(1U << 24)
#define SPIF_ADDR_SIZE_8BIT_V2	(0U << 24)
#define SPIF_ADDR_SIZE_16BIT_V2 (1U << 24)
#define SPIF_ADDR_SIZE_24BIT_V2 (2U << 24)
#define SPIF_ADDR_SIZE_32BIT_V2 (3U << 24)
#define SPIF_ADDR_SIZE_MASK_V2	(3U << 24)
#define SPIF_CMD_LEN_1BYTE	(0U << 26)
#define SPIF_CMD_LEN_2BYTE	(1U << 26)
#define SPIF_CMD_LEN_MASK	(1U << 26)
#define SPIF_DUMMY_NUM_POS	16U
#define SPIF_DATA_NUM_POS	0U
#define SPIF_DES_NORMAL_EN	BIT(28)

#define SPIF_SINGLE_MODE 1U
#define SPIF_DUAL_MODE	 2U
#define SPIF_QUAD_MODE	 4U
#define SPIF_OCTAL_MODE	 8U

/* DTR read opcodes recognized by the SPIF hardware. */
#define SPIF_DTR_READ_1_1_1    0x0dU
#define SPIF_DTR_READ_1_2_2    0xbdU
#define SPIF_DTR_READ_1_4_4    0xedU
#define SPIF_DTR_READ_1_1_1_4B 0x0eU
#define SPIF_DTR_READ_1_2_2_4B 0xbeU
#define SPIF_DTR_READ_1_4_4_4B 0xeeU

#endif /* __DRIVERS_SPIF_REGS_H__ */
