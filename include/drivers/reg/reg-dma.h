/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __REG_DMA_H__
#define __REG_DMA_H__

#include <reg-ncat.h>

#ifdef __cplusplus
extern "C" {
#endif// __cplusplus

#define SUNXI_DMA_CHANNEL_BASE (SUNXI_DMA_BASE + 0x100)
#define DMA_AUTO_GATE_REG (SUNXI_DMA_BASE + 0x28)

#define SUNXI_DMA_CHANNEL_SIZE (0x40)
#define SUNXI_DMA_LINK_NULL (0xfffff800)

#define DMAC_DMATYPE_NORMAL 0
#define DMAC_CFG_TYPE_DRAM (1)
#define DMAC_CFG_TYPE_SRAM (0)

#define DMAC_CFG_TYPE_SPI0 (22)
#define DMAC_CFG_TYPE_SHMC0 (20)
#define DMAC_CFG_SRC_TYPE_NAND (5)

/* DMA base config  */
#define DMAC_CFG_CONTINUOUS_ENABLE (0x01)
#define DMAC_CFG_CONTINUOUS_DISABLE (0x00)

/* DMA dest width config */
#define DMAC_CFG_DEST_DATA_WIDTH_8BIT (0x00)
#define DMAC_CFG_DEST_DATA_WIDTH_16BIT (0x01)
#define DMAC_CFG_DEST_DATA_WIDTH_32BIT (0x02)
#define DMAC_CFG_DEST_DATA_WIDTH_64BIT (0x03)

/* DMA dest bust config */
#define DMAC_CFG_DEST_1_BURST (0x00)
#define DMAC_CFG_DEST_4_BURST (0x01)
#define DMAC_CFG_DEST_8_BURST (0x02)
#define DMAC_CFG_DEST_16_BURST (0x03)

#define DMAC_CFG_DEST_ADDR_TYPE_LINEAR_MODE (0x00)
#define DMAC_CFG_DEST_ADDR_TYPE_IO_MODE (0x01)

/* DMA src config */
#define DMAC_CFG_SRC_DATA_WIDTH_8BIT (0x00)
#define DMAC_CFG_SRC_DATA_WIDTH_16BIT (0x01)
#define DMAC_CFG_SRC_DATA_WIDTH_32BIT (0x02)
#define DMAC_CFG_SRC_DATA_WIDTH_64BIT (0x03)

#define DMAC_CFG_SRC_1_BURST (0x00)
#define DMAC_CFG_SRC_4_BURST (0x01)
#define DMAC_CFG_SRC_8_BURST (0x02)
#define DMAC_CFG_SRC_16_BURST (0x03)

#define DMAC_CFG_SRC_ADDR_TYPE_LINEAR_MODE (0x00)
#define DMAC_CFG_SRC_ADDR_TYPE_IO_MODE (0x01)

/* DMA int config */
#define DMA_PKG_HALF_INT (1 << 0)
#define DMA_PKG_END_INT (1 << 1)
#define DMA_QUEUE_END_INT (1 << 2)

#ifdef __cplusplus
}
#endif// __cplusplus

#endif// __REG_DMA_H__