/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __SDHCI_SDCARD_H__
#define __SDHCI_SDCARD_H__

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include "sys-sdhci.h"

#include "log.h"

#ifdef __cplusplus
extern "C" {
#endif// __cplusplus

enum {
	/* Class 1 */
	MMC_GO_IDLE_STATE = 0,
	MMC_SEND_OP_COND = 1,
	MMC_ALL_SEND_CID = 2,
	MMC_SET_RELATIVE_ADDR = 3,
	MMC_SET_DSR = 4,
	MMC_SWITCH = 6,
	MMC_SELECT_CARD = 7,
	MMC_SEND_EXT_CSD = 8,
	MMC_SEND_CSD = 9,
	MMC_SEND_CID = 10,
	MMC_READ_DAT_UNTIL_STOP = 11,
	MMC_STOP_TRANSMISSION = 12,
	MMC_SEND_STATUS = 13,
	MMC_GO_INACTIVE_STATE = 15,
	MMC_SPI_READ_OCR = 58,
	MMC_SPI_CRC_ON_OFF = 59,

	/* Class 2 */
	MMC_SET_BLOCKLEN = 16,
	MMC_READ_SINGLE_BLOCK = 17,
	MMC_READ_MULTIPLE_BLOCK = 18,

	/* Class 3 */
	MMC_WRITE_DAT_UNTIL_STOP = 20,

	/* Class 4 */
	MMC_SET_BLOCK_COUNT = 23,
	MMC_WRITE_SINGLE_BLOCK = 24,
	MMC_WRITE_MULTIPLE_BLOCK = 25,
	MMC_PROGRAM_CID = 26,
	MMC_PROGRAM_CSD = 27,

	/* Class 5 */
	MMC_ERASE_GROUP_START = 35,
	MMC_ERASE_GROUP_END = 36,
	MMC_ERASE = 38,

	/* Class 6 */
	MMC_SET_WRITE_PROT = 28,
	MMC_CLR_WRITE_PROT = 29,
	MMC_SEND_WRITE_PROT = 30,

	/* Class 7 */
	MMC_LOCK_UNLOCK = 42,

	/* Class 8 */
	MMC_APP_CMD = 55,
	MMC_GEN_CMD = 56,

	/* Class 9 */
	MMC_FAST_IO = 39,
	MMC_GO_IRQ_STATE = 40,
};

enum {
	SD_CMD_SEND_RELATIVE_ADDR = 3,
	SD_CMD_SWITCH_FUNC = 6,
	SD_CMD_SEND_IF_COND = 8,
	SD_CMD_APP_SET_BUS_WIDTH = 6,
	SD_CMD_ERASE_WR_BLK_START = 32,
	SD_CMD_ERASE_WR_BLK_END = 33,
	SD_CMD_APP_SEND_OP_COND = 41,
	SD_CMD_APP_SEND_SCR = 51,
};

enum {
	MMC_RSP_PRESENT = (1 << 0),
	MMC_RSP_136 = (1 << 1),
	MMC_RSP_CRC = (1 << 2),
	MMC_RSP_BUSY = (1 << 3),
	MMC_RSP_OPCODE = (1 << 4),
};

enum {
	MMC_RSP_NONE = (0 << 24),
	MMC_RSP_R1 = (1 << 24) | (MMC_RSP_PRESENT | MMC_RSP_CRC | MMC_RSP_OPCODE),
	MMC_RSP_R1B = (1 << 24) | (MMC_RSP_PRESENT | MMC_RSP_CRC | MMC_RSP_OPCODE | MMC_RSP_BUSY),
	MMC_RSP_R2 = (2 << 24) | (MMC_RSP_PRESENT | MMC_RSP_136 | MMC_RSP_CRC),
	MMC_RSP_R3 = (3 << 24) | (MMC_RSP_PRESENT),
	MMC_RSP_R4 = (4 << 24) | (MMC_RSP_PRESENT),
	MMC_RSP_R5 = (5 << 24) | (MMC_RSP_PRESENT | MMC_RSP_CRC | MMC_RSP_OPCODE),
	MMC_RSP_R6 = (6 << 24) | (MMC_RSP_PRESENT | MMC_RSP_CRC | MMC_RSP_OPCODE),
	MMC_RSP_R7 = (7 << 24) | (MMC_RSP_PRESENT | MMC_RSP_CRC | MMC_RSP_OPCODE),
};

enum {
	MMC_STATUS_IDLE = 0,
	MMC_STATUS_READY = 1,
	MMC_STATUS_IDENT = 2,
	MMC_STATUS_STBY = 3,
	MMC_STATUS_TRAN = 4,
	MMC_STATUS_DATA = 5,
	MMC_STATUS_RCV = 6,
	MMC_STATUS_PRG = 7,
	MMC_STATUS_DIS = 8,
	MMC_STATUS_BTST = 9,
	MMC_STATUS_SLP = 10,
};

enum {
	OCR_BUSY = 0x80000000,
	OCR_HCS = 0x40000000,
	OCR_VOLTAGE_MASK = 0x00ffff80,
	OCR_ACCESS_MODE = 0x60000000,
};

enum {
	MMC_DATA_READ = (1 << 0),
	MMC_DATA_WRITE = (1 << 1),
};

enum {
	MMC_VDD_27_36 = (1 << 0),
	MMC_VDD_165_195 = (1 << 1),
};

enum {
	MMC_BUS_WIDTH_1 = 1,
	MMC_BUS_WIDTH_4 = 2,
	MMC_BUS_WIDTH_8 = 3,
};

enum {
	MMC_CONTROLLER_0 = 0,
	MMC_CONTROLLER_1 = 1,
	MMC_CONTROLLER_2 = 2,
};

enum {
	SD_VERSION_SD = 0x20000,
	SD_VERSION_3 = (SD_VERSION_SD | 0x300),
	SD_VERSION_2 = (SD_VERSION_SD | 0x200),
	SD_VERSION_1_0 = (SD_VERSION_SD | 0x100),
	SD_VERSION_1_10 = (SD_VERSION_SD | 0x10a),
	MMC_VERSION_MMC = 0x10000,
	MMC_VERSION_UNKNOWN = (MMC_VERSION_MMC),
	MMC_VERSION_1_2 = (MMC_VERSION_MMC | 0x102),
	MMC_VERSION_1_4 = (MMC_VERSION_MMC | 0x104),
	MMC_VERSION_2_2 = (MMC_VERSION_MMC | 0x202),
	MMC_VERSION_3 = (MMC_VERSION_MMC | 0x300),
	MMC_VERSION_4 = (MMC_VERSION_MMC | 0x400),
	MMC_VERSION_4_1 = (MMC_VERSION_MMC | 0x401),
	MMC_VERSION_4_2 = (MMC_VERSION_MMC | 0x402),
	MMC_VERSION_4_3 = (MMC_VERSION_MMC | 0x403),
	MMC_VERSION_4_41 = (MMC_VERSION_MMC | 0x429),
	MMC_VERSION_4_5 = (MMC_VERSION_MMC | 0x405),
	MMC_VERSION_5_0 = (MMC_VERSION_MMC | 0x500),
	MMC_VERSION_5_1 = (MMC_VERSION_MMC | 0x501),
};

typedef struct {
	uint32_t version;
	uint32_t ocr;
	uint32_t rca;
	uint32_t cid[4];
	uint32_t csd[4];
	uint8_t extcsd[512];

	uint32_t high_capacity;
	uint32_t tran_speed;
	uint32_t dsr_imp;
	uint32_t read_bl_len;
	uint32_t write_bl_len;
	uint64_t capacity;
} sdmmc_t;

typedef struct {
	sdmmc_t card;
	sdhci_t *hci;
	uint8_t buf[512];
	bool online;
} sdmmc_pdata_t;

/**
 * External declaration of the 'card0' SDMMC platform data structure.
 */
extern sdmmc_pdata_t card0;

/**
 * Initialize the SDMMC controller with the specified platform data and SDHCI driver.
 *
 * @param data Pointer to the SDMMC platform data structure.
 * @param hci Pointer to the SDHCI driver instance.
 * @return 0 if successful, or an error code if failed.
 */
int sdmmc_init(sdmmc_pdata_t *data, sdhci_t *hci);

/**
 * Read data from the SDMMC card into the provided buffer.
 *
 * @param data Pointer to the SDMMC platform data structure.
 * @param buf Pointer to the destination buffer to store the read data.
 * @param blkno The starting block number to read from.
 * @param blkcnt The number of blocks to read.
 * @return The total number of bytes read, or an error code if failed.
 */
uint64_t sdmmc_blk_read(sdmmc_pdata_t *data, uint8_t *buf, uint64_t blkno, uint64_t blkcnt);

#ifdef __cplusplus
}
#endif// __cplusplus

#endif /* __SDHCI_SDCARD_H__ */
