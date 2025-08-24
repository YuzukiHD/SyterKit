/* SPDX-License-Identifier: GPL-2.0+ */

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>

#include <common.h>

#include "sys-sdcard.h"

#define FALSE 0
#define TRUE 1

/*
 * EXT_CSD fields
 */

#define EXT_CSD_CMDQ_MODE_EN 15					/* R/W */
#define EXT_CSD_FLUSH_CACHE 32					/* W */
#define EXT_CSD_CACHE_CTRL 33					/* R/W */
#define EXT_CSD_POWER_OFF_NOTIFICATION 34		/* R/W */
#define EXT_CSD_PACKED_FAILURE_INDEX 35			/* RO */
#define EXT_CSD_PACKED_CMD_STATUS 36			/* RO */
#define EXT_CSD_EXP_EVENTS_STATUS 54			/* RO, 2 bytes */
#define EXT_CSD_EXP_EVENTS_CTRL 56				/* R/W, 2 bytes */
#define EXT_CSD_DATA_SECTOR_SIZE 61				/* R */
#define EXT_CSD_GP_SIZE_MULT 143				/* R/W */
#define EXT_CSD_PARTITION_SETTING_COMPLETED 155 /* R/W */
#define EXT_CSD_PARTITION_ATTRIBUTE 156			/* R/W */
#define EXT_CSD_PARTITION_SUPPORT 160			/* RO */
#define EXT_CSD_HPI_MGMT 161					/* R/W */
#define EXT_CSD_RST_N_FUNCTION 162				/* R/W */
#define EXT_CSD_BKOPS_EN 163					/* R/W */
#define EXT_CSD_BKOPS_START 164					/* W */
#define EXT_CSD_SANITIZE_START 165				/* W */
#define EXT_CSD_WR_REL_PARAM 166				/* RO */
#define EXT_CSD_RPMB_MULT 168					/* RO */
#define EXT_CSD_FW_CONFIG 169					/* R/W */
#define EXT_CSD_BOOT_WP 173						/* R/W */
#define EXT_CSD_ERASE_GROUP_DEF 175				/* R/W */
#define EXT_CSD_PART_CONFIG 179					/* R/W */
#define EXT_CSD_ERASED_MEM_CONT 181				/* RO */
#define EXT_CSD_BUS_WIDTH 183					/* R/W */
#define EXT_CSD_STROBE_SUPPORT 184				/* RO */
#define EXT_CSD_HS_TIMING 185					/* R/W */
#define EXT_CSD_POWER_CLASS 187					/* R/W */
#define EXT_CSD_REV 192							/* RO */
#define EXT_CSD_STRUCTURE 194					/* RO */
#define EXT_CSD_CARD_TYPE 196					/* RO */
#define EXT_CSD_DRIVER_STRENGTH 197				/* RO */
#define EXT_CSD_OUT_OF_INTERRUPT_TIME 198		/* RO */
#define EXT_CSD_PART_SWITCH_TIME 199			/* RO */
#define EXT_CSD_PWR_CL_52_195 200				/* RO */
#define EXT_CSD_PWR_CL_26_195 201				/* RO */
#define EXT_CSD_PWR_CL_52_360 202				/* RO */
#define EXT_CSD_PWR_CL_26_360 203				/* RO */
#define EXT_CSD_SEC_CNT 212						/* RO, 4 bytes */
#define EXT_CSD_S_A_TIMEOUT 217					/* RO */
#define EXT_CSD_REL_WR_SEC_C 222				/* RO */
#define EXT_CSD_HC_WP_GRP_SIZE 221				/* RO */
#define EXT_CSD_ERASE_TIMEOUT_MULT 223			/* RO */
#define EXT_CSD_HC_ERASE_GRP_SIZE 224			/* RO */
#define EXT_CSD_BOOT_MULT 226					/* RO */
#define EXT_CSD_SEC_TRIM_MULT 229				/* RO */
#define EXT_CSD_SEC_ERASE_MULT 230				/* RO */
#define EXT_CSD_SEC_FEATURE_SUPPORT 231			/* RO */
#define EXT_CSD_TRIM_MULT 232					/* RO */
#define EXT_CSD_PWR_CL_200_195 236				/* RO */
#define EXT_CSD_PWR_CL_200_360 237				/* RO */
#define EXT_CSD_PWR_CL_DDR_52_195 238			/* RO */
#define EXT_CSD_PWR_CL_DDR_52_360 239			/* RO */
#define EXT_CSD_BKOPS_STATUS 246				/* RO */
#define EXT_CSD_POWER_OFF_LONG_TIME 247			/* RO */
#define EXT_CSD_GENERIC_CMD6_TIME 248			/* RO */
#define EXT_CSD_CACHE_SIZE 249					/* RO, 4 bytes */
#define EXT_CSD_PWR_CL_DDR_200_360 253			/* RO */
#define EXT_CSD_FIRMWARE_VERSION 254			/* RO, 8 bytes */
#define EXT_CSD_PRE_EOL_INFO 267				/* RO */
#define EXT_CSD_DEVICE_LIFE_TIME_EST_TYP_A 268	/* RO */
#define EXT_CSD_DEVICE_LIFE_TIME_EST_TYP_B 269	/* RO */
#define EXT_CSD_CMDQ_DEPTH 307					/* RO */
#define EXT_CSD_CMDQ_SUPPORT 308				/* RO */
#define EXT_CSD_SUPPORTED_MODE 493				/* RO */
#define EXT_CSD_TAG_UNIT_SIZE 498				/* RO */
#define EXT_CSD_DATA_TAG_SUPPORT 499			/* RO */
#define EXT_CSD_MAX_PACKED_WRITES 500			/* RO */
#define EXT_CSD_MAX_PACKED_READS 501			/* RO */
#define EXT_CSD_BKOPS_SUPPORT 502				/* RO */
#define EXT_CSD_HPI_FEATURES 503				/* RO */

#define EXT_CSD_BUS_WIDTH_1 0	  /* Card is in 1 bit mode */
#define EXT_CSD_BUS_WIDTH_4 1	  /* Card is in 4 bit mode */
#define EXT_CSD_BUS_WIDTH_8 2	  /* Card is in 8 bit mode */
#define EXT_CSD_DDR_BUS_WIDTH_4 5 /* Card is in 4 bit DDR mode */
#define EXT_CSD_DDR_BUS_WIDTH_8 6 /* Card is in 8 bit DDR mode */

#define EXT_CSD_CARD_TYPE_26 (1 << 0)		/* Card can run at 26MHz */
#define EXT_CSD_CARD_TYPE_52 (1 << 1)		/* Card can run at 52MHz */
#define EXT_CSD_CARD_TYPE_MASK 0x3F			/* Mask out reserved bits */
#define EXT_CSD_CARD_TYPE_DDR_1_8V (1 << 2) /* Card can run at 52MHz */
/* DDR mode @1.8V or 3V I/O */
#define EXT_CSD_CARD_TYPE_DDR_1_2V (1 << 3) /* Card can run at 52MHz */
/* DDR mode @1.2V I/O */
#define EXT_CSD_CARD_TYPE_DDR_52 (EXT_CSD_CARD_TYPE_DDR_1_8V | EXT_CSD_CARD_TYPE_DDR_1_2V)
#define EXT_CSD_CARD_TYPE_SDR_1_8V (1 << 4) /* Card can run at 200MHz */
#define EXT_CSD_CARD_TYPE_SDR_1_2V (1 << 5) /* Card can run at 200MHz */
/* SDR mode @1.2V I/O */

#define EXT_CSD_CMD_SET_NORMAL (1 << 0)
#define EXT_CSD_CMD_SET_SECURE (1 << 1)
#define EXT_CSD_CMD_SET_CPSECURE (1 << 2)

#define EXT_CSD_PWR_CL_8BIT_MASK 0xF0 /* 8 bit PWR CLS */
#define EXT_CSD_PWR_CL_4BIT_MASK 0x0F /* 8 bit PWR CLS */
#define EXT_CSD_PWR_CL_8BIT_SHIFT 4
#define EXT_CSD_PWR_CL_4BIT_SHIFT 0

sdmmc_pdata_t card0;

#define UNSTUFF_BITS(resp, start, size)                              \
	({                                                               \
		const int __size = size;                                     \
		const uint32_t __mask = (__size < 32 ? 1 << __size : 0) - 1; \
		const int __off = 3 - ((start) / 32);                        \
		const int __shft = (start) &31;                              \
		uint32_t __res;                                              \
                                                                     \
		__res = resp[__off] >> __shft;                               \
		if (__size + __shft > 32)                                    \
			__res |= resp[__off - 1] << ((32 - __shft) % 32);        \
		__res &__mask;                                               \
	})

static const unsigned tran_speed_unit[] = {
		[0] = 10000,
		[1] = 100000,
		[2] = 1000000,
		[3] = 10000000,
};

static const unsigned char tran_speed_time[] = {
		0,
		10,
		12,
		13,
		15,
		20,
		25,
		30,
		35,
		40,
		45,
		50,
		55,
		60,
		70,
		80,
};

static uint32_t extract_mid(sdmmc_t *card) {
	if ((card->version & MMC_VERSION_MMC) && (card->version <= MMC_VERSION_1_4))
		return UNSTUFF_BITS(card->cid, 104, 24);
	else
		return UNSTUFF_BITS(card->cid, 120, 8);
}

static uint32_t extract_oid(sdmmc_t *card) {
	return (card->cid[0] >> 8) & 0xffff;
}

static uint32_t extract_prv(sdmmc_t *card) {
	return (card->cid[2] >> 24);
}

static uint32_t extract_psn(sdmmc_t *card) {
	if (card->version & SD_VERSION_SD) {
		return UNSTUFF_BITS(card->csd, 24, 32);
	} else {
		if (card->version > MMC_VERSION_1_4)
			return UNSTUFF_BITS(card->cid, 16, 32);
		else
			return UNSTUFF_BITS(card->cid, 16, 24);
	}
}

static uint32_t extract_month(sdmmc_t *card) {
	if (card->version & SD_VERSION_SD)
		return UNSTUFF_BITS(card->cid, 8, 4);
	else
		return UNSTUFF_BITS(card->cid, 12, 4);
}

static uint32_t extract_year(sdmmc_t *card) {
	uint32_t year;

	if (card->version & SD_VERSION_SD)
		year = UNSTUFF_BITS(card->cid, 12, 8) + 2000;
	else if (card->version < MMC_VERSION_4_41)
		return UNSTUFF_BITS(card->cid, 8, 4) + 1997;
	else {
		year = UNSTUFF_BITS(card->cid, 8, 4) + 1997;
		if (year < 2010)
			year += 16;
	}
	return year;
}

static bool go_idle_state(sdhci_t *hci) {
	sdhci_cmd_t cmd = {0};

	// CMD0 0x0 for user partition
	cmd.idx = MMC_GO_IDLE_STATE;
	cmd.arg = 0;
	cmd.resptype = hci->isspi ? MMC_RSP_R1 : MMC_RSP_NONE;

	return sdhci_transfer(hci, &cmd, NULL);
}

static bool sd_send_if_cond(sdhci_t *hci, sdmmc_t *card) {
	sdhci_cmd_t cmd = {0};

	cmd.idx = SD_CMD_SEND_IF_COND;
	if (hci->voltage & MMC_VDD_27_36)
		cmd.arg = (0x1 << 8);
	else if (hci->voltage & MMC_VDD_165_195)
		cmd.arg = (0x2 << 8);
	else
		cmd.arg = (0x0 << 8);
	cmd.arg |= 0xaa;
	cmd.resptype = MMC_RSP_R7;
	if (!sdhci_transfer(hci, &cmd, NULL))
		return FALSE;

	if ((cmd.response[0] & 0xff) != 0xaa)
		return FALSE;
	card->version = SD_VERSION_2;
	return TRUE;
}

static bool sd_send_op_cond(sdhci_t *hci, sdmmc_t *card) {
	sdhci_cmd_t cmd = {0};
	int retries = 50;

	if (!go_idle_state(hci))
		return FALSE;

	if (!sd_send_if_cond(hci, card)) {
		return FALSE;
	}

	do {
		cmd.idx = MMC_APP_CMD;
		cmd.arg = 0;
		cmd.resptype = MMC_RSP_R1;
		if (!sdhci_transfer(hci, &cmd, NULL))
			continue;

		cmd.idx = SD_CMD_APP_SEND_OP_COND;
		if (hci->isspi) {
			cmd.arg = 0;
			if (card->version == SD_VERSION_2)
				cmd.arg |= OCR_HCS;
			cmd.resptype = MMC_RSP_R1;
			if (sdhci_transfer(hci, &cmd, NULL))
				break;
		} else {
			if (hci->voltage & MMC_VDD_27_36)
				cmd.arg = 0x00ff8000;
			else if (hci->voltage & MMC_VDD_165_195)
				cmd.arg = 0x00000080;
			else
				cmd.arg = 0;
			if (card->version == SD_VERSION_2)
				cmd.arg |= OCR_HCS;
			cmd.resptype = MMC_RSP_R3;
			if (!sdhci_transfer(hci, &cmd, NULL) || (cmd.response[0] & OCR_BUSY))
				break;
		}
	} while (retries--);

	if (retries <= 0)
		return FALSE;

	if (card->version != SD_VERSION_2)
		card->version = SD_VERSION_1_0;
	if (hci->isspi) {
		cmd.idx = MMC_SPI_READ_OCR;
		cmd.arg = 0;
		cmd.resptype = MMC_RSP_R3;
		if (!sdhci_transfer(hci, &cmd, NULL))
			return FALSE;
	}
	card->ocr = cmd.response[0];
	card->high_capacity = ((card->ocr & OCR_HCS) == OCR_HCS);
	card->rca = 0;

	return TRUE;
}

static bool mmc_send_op_cond(sdhci_t *hci, sdmmc_t *card) {
	sdhci_cmd_t cmd = {0};
	int retries = 50;

	cmd.idx = MMC_SEND_OP_COND;
	cmd.resptype = MMC_RSP_R3;
	if (hci->voltage == MMC_VDD_27_36) {
		cmd.arg = 0x40FF8000;// Sector access mode, 2.7-3.6v VCCQ
	} else if (hci->voltage == MMC_VDD_165_195) {
		cmd.arg = 0x40FF0080;// Sector access mode, 1.65-1.95v VCCQ
	}

	do {
		cmd.response[0] = 0;
		if (!sdhci_transfer(hci, &cmd, NULL)) {
			return FALSE;
		}
		udelay(5000);
	} while (!(cmd.response[0] & OCR_BUSY) && retries--);
	printk_trace("SHMC: op_cond 0x%x, retries = %d\n", cmd.response[0], retries);

	if (retries <= 0)
		return FALSE;

	if (hci->isspi) {
		cmd.idx = MMC_SPI_READ_OCR;
		cmd.arg = 0;
		cmd.resptype = MMC_RSP_R3;
		if (!sdhci_transfer(hci, &cmd, NULL))
			return FALSE;
	}
	card->version = MMC_VERSION_UNKNOWN;
	card->ocr = cmd.response[0];
	card->high_capacity = ((card->ocr & OCR_HCS) == OCR_HCS);
	card->rca = 1;
	return TRUE;
}

static int sdmmc_status(sdhci_t *hci, sdmmc_t *card) {
	sdhci_cmd_t cmd = {0};
	int retries = 100;

	cmd.idx = MMC_SEND_STATUS;
	cmd.resptype = MMC_RSP_R1;
	cmd.arg = card->rca << 16;
	do {
		if (!sdhci_transfer(hci, &cmd, NULL))
			continue;
		if (cmd.response[0] & (1 << 8))
			break;
		udelay(1);
	} while (retries-- > 0);
	if (retries > 0)
		return ((cmd.response[0] >> 9) & 0xf);
	printk_warning("SMHC: status error\n");
	return -1;
}

static uint64_t sdmmc_read_blocks(sdhci_t *hci, sdmmc_t *card, uint8_t *buf, uint64_t start, uint64_t blkcnt) {
	sdhci_cmd_t cmd = {0};
	sdhci_data_t dat = {0};
	int status;

	if (blkcnt > 1)
		cmd.idx = MMC_READ_MULTIPLE_BLOCK;
	else
		cmd.idx = MMC_READ_SINGLE_BLOCK;
	if (card->high_capacity)
		cmd.arg = start;
	else
		cmd.arg = start * card->read_bl_len;
	cmd.resptype = MMC_RSP_R1;
	dat.buf = buf;
	dat.flag = MMC_DATA_READ;
	dat.blksz = card->read_bl_len;
	dat.blkcnt = blkcnt;

	if (!sdhci_transfer(hci, &cmd, &dat)) {
		printk_warning("SMHC: read failed\n");
		return 0;
	}

	if (!hci->isspi) {
		do {
			status = sdmmc_status(hci, card);
			if (status < 0) {
				return 0;
			}
		} while ((status != MMC_STATUS_TRAN) && (status != MMC_STATUS_DATA));
	}

	if (blkcnt > 1) {
		cmd.idx = MMC_STOP_TRANSMISSION;
		cmd.arg = 0;
		cmd.resptype = MMC_RSP_R1B;
		if (!sdhci_transfer(hci, &cmd, NULL)) {
			printk_warning("SMHC: transfer stop failed\n");
			return 0;
		}
	}
	return blkcnt;
}

static uint64_t sdmmc_write_blocks(sdhci_t *hci, sdmmc_t *card, uint8_t *buf, uint64_t start, uint64_t blkcnt) {
	sdhci_cmd_t cmd = {0};
	sdhci_data_t dat = {0};
	int status;

	if (blkcnt > 1)
		cmd.idx = MMC_WRITE_MULTIPLE_BLOCK;
	else
		cmd.idx = MMC_WRITE_SINGLE_BLOCK;
	if (card->high_capacity)
		cmd.arg = start;
	else
		cmd.arg = start * card->write_bl_len;
	cmd.resptype = MMC_RSP_R1;
	dat.buf = buf;
	dat.flag = MMC_DATA_WRITE;
	dat.blksz = card->write_bl_len;
	dat.blkcnt = blkcnt;

	if (!sdhci_transfer(hci, &cmd, &dat)) {
		printk_warning("SMHC: write failed\n");
		return 0;
	}

	if (!hci->isspi) {
		do {
			status = sdmmc_status(hci, card);
			if (status < 0)
				return 0;
		} while ((status != MMC_STATUS_TRAN) && (status != MMC_STATUS_RCV));
	}

	if (blkcnt > 1) {
		cmd.idx = MMC_STOP_TRANSMISSION;
		cmd.arg = 0;
		cmd.resptype = MMC_RSP_R1B;
		if (!sdhci_transfer(hci, &cmd, NULL))
			printk_warning("SMHC: transfer stop failed\n");
		return 0;
	}
	return blkcnt;
}

static bool sdmmc_detect(sdhci_t *hci, sdmmc_t *card) {
	sdhci_cmd_t cmd = {0};
	sdhci_data_t dat = {0};
	uint64_t csize, cmult;
	uint32_t unit, time;
	int width;
	int status;

	sdhci_reset(hci);
	if (!sdhci_set_clock(hci, MMC_CLK_400K) || !sdhci_set_width(hci, MMC_BUS_WIDTH_1)) {
		printk_error("SMHC: set clock/width failed\n");
		return FALSE;
	}

	if (!go_idle_state(hci)) {
		printk_error("SMHC: set idle state failed\n");
		return FALSE;
	}
	udelay(2000);// 1ms + 74 clocks @ 400KHz (185us)

	// if set the sdio type, use it
	if (hci->sdio_type == SDHCI_TYPE_SD) {
		if (!sd_send_op_cond(hci, card)) {
			printk_info("SMHC: SD detect failed\n");
			return FALSE;
		}

	} else if (hci->sdio_type == SDHCI_TYPE_MMC) {
		if (!mmc_send_op_cond(hci, card)) {
			printk_info("SMHC: MMC detect failed\n");
			return FALSE;
		}
	} else {
		// Not set sdio_type, SD & MMC: try SD first
		// Otherwise there's only one media type if enabled
		if (!sd_send_op_cond(hci, card)) {
			printk_info("SMHC: SD detect failed, retrying MMC\n");
			sdhci_reset(hci);
			sdhci_set_clock(hci, MMC_CLK_400K);
			sdhci_set_width(hci, MMC_BUS_WIDTH_1);

			if (!go_idle_state(hci)) {
				printk_error("SMHC: set idle state failed\n");
				return FALSE;
			}
			udelay(2000);// 1ms + 74 clocks @ 400KHz (185us)

			if (!mmc_send_op_cond(hci, card)) {
				printk_info("SMHC: SD/MMC detect failed\n");
				return FALSE;
			}
		}
	}

	if (hci->isspi) {
		cmd.idx = MMC_SEND_CID;
		cmd.arg = 0;
		cmd.resptype = MMC_RSP_R1;
		if (!sdhci_transfer(hci, &cmd, NULL))
			return FALSE;
		card->cid[0] = cmd.response[0];
		card->cid[1] = cmd.response[1];
		card->cid[2] = cmd.response[2];
		card->cid[3] = cmd.response[3];

		cmd.idx = MMC_SEND_CSD;
		cmd.arg = 0;
		cmd.resptype = MMC_RSP_R1;
		if (!sdhci_transfer(hci, &cmd, NULL))
			return FALSE;
		card->csd[0] = cmd.response[0];
		card->csd[1] = cmd.response[1];
		card->csd[2] = cmd.response[2];
		card->csd[3] = cmd.response[3];
	} else {
		cmd.idx = MMC_ALL_SEND_CID;
		cmd.arg = 0;
		cmd.resptype = MMC_RSP_R2;
		if (!sdhci_transfer(hci, &cmd, NULL))
			return FALSE;
		card->cid[0] = cmd.response[0];
		card->cid[1] = cmd.response[1];
		card->cid[2] = cmd.response[2];
		card->cid[3] = cmd.response[3];

		cmd.idx = SD_CMD_SEND_RELATIVE_ADDR;
		cmd.arg = card->rca << 16;
		cmd.resptype = MMC_RSP_R6;
		if (!sdhci_transfer(hci, &cmd, NULL))
			return FALSE;
		if (card->version & SD_VERSION_SD)
			card->rca = (cmd.response[0] >> 16) & 0xffff;

		cmd.idx = MMC_SEND_CSD;
		cmd.arg = card->rca << 16;
		cmd.resptype = MMC_RSP_R2;
		if (!sdhci_transfer(hci, &cmd, NULL))
			return FALSE;
		card->csd[0] = cmd.response[0];
		card->csd[1] = cmd.response[1];
		card->csd[2] = cmd.response[2];
		card->csd[3] = cmd.response[3];

		cmd.idx = MMC_SELECT_CARD;
		cmd.arg = card->rca << 16;
		cmd.resptype = MMC_RSP_R1;
		if (!sdhci_transfer(hci, &cmd, NULL))
			return FALSE;
		do {
			status = sdmmc_status(hci, card);
			if (status < 0)
				return FALSE;
		} while (status != MMC_STATUS_TRAN);
	}

	if (card->version == MMC_VERSION_UNKNOWN) {
		switch ((card->csd[0] >> 26) & 0xf) {
			case 0:
				card->version = MMC_VERSION_1_2;
				break;
			case 1:
				card->version = MMC_VERSION_1_4;
				break;
			case 2:
				card->version = MMC_VERSION_2_2;
				break;
			case 3:
				card->version = MMC_VERSION_3;
				break;
			case 4:
				card->version = MMC_VERSION_4;
				break;
			default:
				card->version = MMC_VERSION_1_2;
				break;
		};
	}

	unit = tran_speed_unit[(card->csd[0] & 0x7)];
	time = tran_speed_time[((card->csd[0] >> 3) & 0xf)];
	card->tran_speed = time * unit;
	card->dsr_imp = UNSTUFF_BITS(card->csd, 76, 1);

	card->read_bl_len = 1 << UNSTUFF_BITS(card->csd, 80, 4);
	if (card->version & SD_VERSION_SD)
		card->write_bl_len = card->read_bl_len;
	else
		card->write_bl_len = 1 << ((card->csd[3] >> 22) & 0xf);
	if (card->read_bl_len > 512)
		card->read_bl_len = 512;
	if (card->write_bl_len > 512)
		card->write_bl_len = 512;

	if ((card->version & MMC_VERSION_MMC) && (card->version >= MMC_VERSION_4)) {
		card->tran_speed = 50000000;
		cmd.idx = MMC_SEND_EXT_CSD;
		cmd.arg = 0;
		cmd.resptype = MMC_RSP_R1;
		dat.buf = card->extcsd;
		dat.flag = MMC_DATA_READ;
		dat.blksz = 512;
		dat.blkcnt = 1;
		if (!sdhci_transfer(hci, &cmd, &dat))
			return FALSE;
		if (!hci->isspi) {
			do {
				status = sdmmc_status(hci, card);
				if (status < 0)
					return FALSE;
			} while (status != MMC_STATUS_TRAN);
		}
		const char *strver = "unknown";
		switch (card->extcsd[EXT_CSD_REV]) {
			case 1:
				card->version = MMC_VERSION_4_1;
				strver = "4.1";
				break;
			case 2:
				card->version = MMC_VERSION_4_2;
				strver = "4.2";
				break;
			case 3:
				card->version = MMC_VERSION_4_3;
				strver = "4.3";
				break;
			case 5:
				card->version = MMC_VERSION_4_41;
				strver = "4.41";
				break;
			case 6:
				card->version = MMC_VERSION_4_5;
				strver = "4.5";
				break;
			case 7:
				card->version = MMC_VERSION_5_0;
				strver = "5.0";
				break;
			case 8:
				card->version = MMC_VERSION_5_1;
				strver = "5.1";
				break;
			default:
				break;
		}
		printk_info("SMHC: MMC version %s\n", strver);
	}

	if (card->high_capacity) {
		if (card->version & SD_VERSION_SD) {
			csize = UNSTUFF_BITS(card->csd, 48, 22);
			card->capacity = (1 + csize) << 10;
		} else {
			card->capacity =
					card->extcsd[EXT_CSD_SEC_CNT] << 0 | card->extcsd[EXT_CSD_SEC_CNT + 1] << 8 | card->extcsd[EXT_CSD_SEC_CNT + 2] << 16 | card->extcsd[EXT_CSD_SEC_CNT + 3] << 24;
		}
	} else {
		cmult = UNSTUFF_BITS(card->csd, 47, 3);
		csize = UNSTUFF_BITS(card->csd, 62, 12);
		card->capacity = (csize + 1) << (cmult + 2);
	}
	card->capacity *= 1 << UNSTUFF_BITS(card->csd, 80, 4);

	if (hci->isspi) {
		if (!sdhci_set_clock(hci, min(card->tran_speed, hci->clock)) || !sdhci_set_width(hci, MMC_BUS_WIDTH_1)) {
			printk_error("SMHC: set clock/width failed\n");
			return FALSE;
		}
	} else {
		if (card->version & SD_VERSION_SD) {
			if (hci->width == MMC_BUS_WIDTH_4)
				width = 2;
			else
				width = 0;

			cmd.idx = MMC_APP_CMD;
			cmd.arg = card->rca << 16;
			cmd.resptype = MMC_RSP_R5;
			if (!sdhci_transfer(hci, &cmd, NULL))
				return FALSE;

			cmd.idx = SD_CMD_SWITCH_FUNC;
			cmd.arg = width;
			cmd.resptype = MMC_RSP_R1;
			if (!sdhci_transfer(hci, &cmd, NULL))
				return FALSE;
		} else if (card->version & MMC_VERSION_MMC) {
			switch (hci->width) {
				case MMC_BUS_WIDTH_8:
					width = EXT_CSD_BUS_WIDTH_8;
					break;
				case MMC_BUS_WIDTH_4:
					if (hci->clock == MMC_CLK_50M_DDR)
						width = EXT_CSD_DDR_BUS_WIDTH_4;
					else
						width = EXT_CSD_BUS_WIDTH_4;
					break;
				default:
					width = EXT_CSD_BUS_WIDTH_1;
					break;
			}

			if (hci->width >= MMC_BUS_WIDTH_4) {
				cmd.idx = SD_CMD_SWITCH_FUNC;
				cmd.resptype = MMC_RSP_R1;
				cmd.arg = (3 << 24) | (EXT_CSD_POWER_CLASS << 16) | EXT_CSD_CMD_SET_NORMAL;
				if (width == EXT_CSD_DDR_BUS_WIDTH_4) {
					cmd.arg |= ((card->extcsd[EXT_CSD_PWR_CL_DDR_52_360] & EXT_CSD_PWR_CL_4BIT_MASK >> EXT_CSD_PWR_CL_4BIT_SHIFT) << 8);
				} else {
					cmd.arg |= ((card->extcsd[EXT_CSD_PWR_CL_52_360] & EXT_CSD_PWR_CL_4BIT_MASK >> EXT_CSD_PWR_CL_4BIT_SHIFT) << 8);
				}

				if (!sdhci_transfer(hci, &cmd, NULL))
					return FALSE;
			}

			// Write EXT_CSD register 183 (width) with our value
			cmd.arg = (3 << 24) | (EXT_CSD_BUS_WIDTH << 16) | (width << 8) | 1;
			if (!sdhci_transfer(hci, &cmd, NULL))
				return FALSE;

			udelay(1000);
		}
		if (!sdhci_set_clock(hci, hci->clock) || !sdhci_set_width(hci, hci->width)) {
			printk_error("SMHC: set clock/width failed\n");
			return FALSE;
		}
	}

	cmd.idx = MMC_SET_BLOCKLEN;
	cmd.arg = card->read_bl_len;
	cmd.resptype = MMC_RSP_R1;
	if (!sdhci_transfer(hci, &cmd, NULL))
		return FALSE;

	printk_debug("SD/MMC card at the '%s' host controller:\r\n", hci->name);
	printk_debug("  Attached is a %s card\r\n", card->version & SD_VERSION_SD ? "SD" : "MMC");
	if (card->capacity / (f64) 1000000000.0 < 4)
		printk_info("  Capacity: %.1fMB\n", (f32) ((f64) card->capacity / (f64) 1000000.0));
	else
		printk_info("  Capacity: %.1fGB\n", (f32) ((f64) card->capacity / (f64) 1000000000.0));
	if (card->high_capacity)
		printk_debug("  High capacity card\r\n");
	printk_debug("  CID: %08X-%08X-%08X-%08X\r\n", card->cid[0], card->cid[1], card->cid[2], card->cid[3]);
	printk_debug("  CSD: %08X-%08X-%08X-%08X\r\n", card->csd[0], card->csd[1], card->csd[2], card->csd[3]);
	printk_debug("  Max transfer speed: %u HZ\r\n", card->tran_speed);
	printk_debug("  Manufacturer ID: %02X\r\n", extract_mid(card));
	printk_debug("  OEM/Application ID: %04X\r\n", extract_oid(card));
	printk_debug("  Product name: '%c%c%c%c%c'\r\n", card->cid[0] & 0xff, (card->cid[1] >> 24), (card->cid[1] >> 16) & 0xff, (card->cid[1] >> 8) & 0xff, card->cid[1] & 0xff);
	printk_debug("  Product revision: %u.%u\r\n", extract_prv(card) >> 4, extract_prv(card) & 0xf);
	printk_debug("  Serial no: %0u\r\n", extract_psn(card));
	printk_debug("  Manufacturing date: %u.%u\r\n", extract_year(card), extract_month(card));

	return TRUE;
}

uint64_t sdmmc_blk_read(sdmmc_pdata_t *data, uint8_t *buf, uint64_t blkno, uint64_t blkcnt) {
	uint64_t cnt, blks = blkcnt;
	sdmmc_t *sdcard = &data->card;

	while (blks > 0) {
		cnt = (blks > 127) ? 127 : blks;
		if (sdmmc_read_blocks(data->hci, sdcard, buf, blkno, cnt) != cnt)
			return 0;
		blks -= cnt;
		blkno += cnt;
		buf += cnt * sdcard->read_bl_len;
	}
	return blkcnt;
}

// TODO: time out bug fix
uint64_t sdmmc_blk_write(sdmmc_pdata_t *data, uint8_t *buf, uint64_t blkno, uint64_t blkcnt) {
	uint64_t cnt, blks = blkcnt;
	sdmmc_t *sdcard = &data->card;

	while (blks > 0) {
		cnt = (blks > 127) ? 127 : blks;
		if (sdmmc_write_blocks(data->hci, sdcard, buf, blkno, cnt) != cnt)
			return 0;
		blks -= cnt;
		blkno += cnt;
		buf += cnt * sdcard->write_bl_len;
	}
	return blkcnt;
}

int sdmmc_init(sdmmc_pdata_t *data, sdhci_t *hci) {
	data->hci = hci;
	data->online = FALSE;

	if (sdmmc_detect(data->hci, &data->card) == TRUE) {
		printk_info("SHMC: %s card detected\n", data->card.version & SD_VERSION_SD ? "SD" : "MMC");
		return 0;
	}

	return -1;
}
