/* SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later */
/*
 * sunxi MIPI DSI host register map (v40 generation).
 *
 * The controller is driven by a small instruction engine: each of the
 * instruction slots describes a lane state (stop, HS, escape, turnaround)
 * and the jump/loop tables chain them into sequences such as "send an LP
 * command" or "stream video in HS".
 */
#ifndef __DISPLAY_DSI_REGS_H__
#define __DISPLAY_DSI_REGS_H__

#include <stdint.h>

#define DISP_BIT(n)		(1U << (n))
#define DISP_GENMASK(h, l)	((0xffffffffU >> (31 - (h))) & (0xffffffffU << (l)))
#define DISP_FIELD_PREP(mask, val) \
	(((uint32_t)(val) << __builtin_ctz(mask)) & (mask))
#define DISP_FIELD_GET(mask, reg) (((reg) & (mask)) >> __builtin_ctz(mask))

#define DSI_CTL				0x000
#define   DSI_CTL_EN			DISP_BIT(0)

#define DSI_GINT0			0x004
#define   DSI_GINT0_EN(irq)		DISP_BIT(irq)
#define   DSI_GINT0_FLAG(irq)		DISP_BIT(16 + (irq))
#define   DSI_IRQ_INSTR_END		0
#define   DSI_IRQ_INSTR_STEP		1
#define   DSI_IRQ_VIDEO_VBLK		2
#define   DSI_IRQ_VIDEO_LINE		3
#define DSI_GINT1			0x008
#define   DSI_GINT1_LINE_NUM		DISP_GENMASK(12, 0)

#define DSI_BASIC_CTL			0x00c
#define   DSI_BASIC_BRDY_L_SEL		DISP_GENMASK(26, 24)
#define   DSI_BASIC_BRDY_SET		DISP_GENMASK(23, 16)
#define   DSI_BASIC_START_MODE		DISP_BIT(8)
#define   DSI_BASIC_TRAIL_INV		DISP_GENMASK(7, 4)
#define   DSI_BASIC_TRAIL_FILL		DISP_BIT(3)
#define   DSI_BASIC_HBP_DIS		DISP_BIT(2)
#define   DSI_BASIC_HSA_HSE_DIS		DISP_BIT(1)
#define   DSI_BASIC_VIDEO_BURST		DISP_BIT(0)

#define DSI_BASIC_CTL0			0x010
#define   DSI_CTL0_HS_EOTP_EN		DISP_BIT(18)
#define   DSI_CTL0_CRC_EN		DISP_BIT(17)
#define   DSI_CTL0_ECC_EN		DISP_BIT(16)
#define   DSI_CTL0_INST_ST		DISP_BIT(0)

#define DSI_BASIC_CTL1			0x014
#define   DSI_CTL1_TRI_DELAY		DISP_GENMASK(31, 16)
#define   DSI_CTL1_VIDEO_START_DELAY	DISP_GENMASK(11, 4)
#define   DSI_CTL1_VIDEO_PREC_ALIGN	DISP_BIT(2)
#define   DSI_CTL1_VIDEO_FRAME_START	DISP_BIT(1)
#define   DSI_CTL1_VIDEO_MODE		DISP_BIT(0)

#define DSI_BASIC_SIZE0			0x018
#define   DSI_SIZE0_VBP			DISP_GENMASK(27, 16)
#define   DSI_SIZE0_VSA			DISP_GENMASK(11, 0)
#define DSI_BASIC_SIZE1			0x01c
#define   DSI_SIZE1_VT			DISP_GENMASK(28, 16)
#define   DSI_SIZE1_VACT		DISP_GENMASK(11, 0)

/* instruction slots 0..7 */
#define DSI_INST_FUNC(n)		(0x020 + (n) * 4)
#define   DSI_INST_MODE			DISP_GENMASK(31, 28)
#define   DSI_INST_ESCAPE_ENTRY		DISP_GENMASK(27, 24)
#define   DSI_INST_PACKET		DISP_GENMASK(23, 20)
#define   DSI_INST_LANE_CEN		DISP_BIT(4)
#define   DSI_INST_LANE_DEN		DISP_GENMASK(3, 0)
#define DSI_INST_LOOP_SEL		0x040
#define DSI_INST_LOOP_NUM		0x044
#define   DSI_INST_LOOP_N1		DISP_GENMASK(27, 16)
#define   DSI_INST_LOOP_N0		DISP_GENMASK(11, 0)
#define DSI_INST_JUMP_SEL		0x048
#define DSI_INST_JUMP_CFG(n)		(0x04c + (n) * 4)
#define   DSI_JUMP_CFG_EN		DISP_BIT(28)
#define   DSI_JUMP_CFG_TO		DISP_GENMASK(23, 20)
#define   DSI_JUMP_CFG_POINT		DISP_GENMASK(19, 16)
#define   DSI_JUMP_CFG_NUM		DISP_GENMASK(15, 0)
#define DSI_INST_LOOP_NUM2		0x054

#define DSI_TRANS_START			0x060
#define DSI_TRANS_ZERO			0x078
#define DSI_TCON_DRQ			0x07c
#define   DSI_DRQ_MODE			DISP_BIT(28)
#define   DSI_DRQ_SET			DISP_GENMASK(9, 0)

#define DSI_PIXEL_CTL0			0x080
#define   DSI_PIXEL_PD_PLUG_DIS		DISP_BIT(16)
#define   DSI_PIXEL_FORMAT		DISP_GENMASK(3, 0)
#define DSI_PIXEL_PH			0x090	/* packet header */
#define DSI_PIXEL_PD			0x094
#define   DSI_PIXEL_PD_TRANN		DISP_GENMASK(23, 16)
#define   DSI_PIXEL_PD_TRAN0		DISP_GENMASK(7, 0)
#define DSI_PIXEL_PF0			0x098
#define DSI_PIXEL_PF1			0x09c

#define DSI_SYNC_HSS			0x0b0
#define DSI_SYNC_HSE			0x0b4
#define DSI_SYNC_VSS			0x0b8
#define DSI_SYNC_VSE			0x0bc
#define DSI_BLK_HSA0			0x0c0
#define DSI_BLK_HSA1			0x0c4
#define DSI_BLK_HBP0			0x0c8
#define DSI_BLK_HBP1			0x0cc
#define DSI_BLK_HFP0			0x0d0
#define DSI_BLK_HFP1			0x0d4
#define DSI_BLK_HBLK0			0x0e0
#define DSI_BLK_HBLK1			0x0e4
#define DSI_BLK_VBLK0			0x0e8
#define DSI_BLK_VBLK1			0x0ec
/* blanking packet payload word: fill byte + CRC of the payload */
#define   DSI_BLK_PF			DISP_GENMASK(31, 16)
#define   DSI_BLK_PD			DISP_GENMASK(7, 0)
#define DSI_BURST_LINE			0x0f0
#define   DSI_BURST_SYNC_POINT		DISP_GENMASK(31, 16)
#define   DSI_BURST_LINE_NUM		DISP_GENMASK(15, 0)
#define DSI_BURST_DRQ			0x0f4
#define   DSI_BURST_DRQ_EDGE1		DISP_GENMASK(31, 16)
#define   DSI_BURST_DRQ_EDGE0		DISP_GENMASK(15, 0)

/* second instruction bank: slots 8..14 */
#define DSI_INST_FUNC1(n)		(0x120 + ((n) - 8) * 4)
#define DSI_INST_LOOP_SEL1		0x140
#define DSI_INST_JUMP_SEL1		0x148

#define DSI_CMD_CTL			0x200
#define   DSI_CMD_RX_OVERFLOW		DISP_BIT(26)
#define   DSI_CMD_RX_FLAG		DISP_BIT(25)
#define   DSI_CMD_RX_SIZE		DISP_GENMASK(20, 16)
#define   DSI_CMD_TX_SIZE		DISP_GENMASK(7, 0)
#define DSI_CMD_RX(n)			(0x240 + (n) * 4)	/* 8 words */
#define DSI_DEBUG_VIDEO0		0x2e0
#define   DSI_DEBUG_CUR_LINE		DISP_GENMASK(12, 0)
#define DSI_DEBUG_INST			0x2f0
#define DSI_DEBUG_DATA			0x2f8
#define DSI_CMD_TX(n)			(0x300 + (n) * 4)	/* 128 words */
#define DSI_CMD_TX_BYTES		512

/* instruction slot numbers */
enum {
	DSI_INST_LP11 = 0,
	DSI_INST_TBA,
	DSI_INST_HSC,
	DSI_INST_HSD,
	DSI_INST_LPDT,
	DSI_INST_HSCEXIT,
	DSI_INST_NOP,
	DSI_INST_DLY,
	DSI_INST_LP11_1,
	DSI_INST_HSC_1,
	DSI_INST_DS_1,
	DSI_INST_LPDT_1,
	DSI_INST_HSCEXIT_1,
	DSI_INST_NOP_1,
	DSI_INST_DLY_1,
	DSI_INST_END = 15,
};

/* instruction modes */
enum {
	DSI_MODE_STOP = 0,
	DSI_MODE_TBA,
	DSI_MODE_HS,
	DSI_MODE_ESCAPE,
	DSI_MODE_HSCEXIT,
	DSI_MODE_NOP,
	DSI_MODE_SCINIT,	/* skew calibration */
};

#define DSI_ESCAPE_LPDT		0
#define DSI_PACKET_PIXEL	0
#define DSI_PACKET_COMMAND	1

#endif /* __DISPLAY_DSI_REGS_H__ */
