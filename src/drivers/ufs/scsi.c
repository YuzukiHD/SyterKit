/* SPDX-License-Identifier: GPL-2.0+ */

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>

#include <ufs/ufs.h>

static scsi_cmd_t scsi_cmd_buffer;
static uint8_t scsi_buffer[512];

static void scsi_print_error(struct scsi_cmd *pccb) {
	int i = 0;
	printk(LOG_LEVEL_MUTE, "UFS: scsi cmd");
	for (i = 0; i < 8; i++) { printk(LOG_LEVEL_MUTE, "%02X ", pccb->cmd[i]); }
	printk(LOG_LEVEL_MUTE, "\n");

	printk(LOG_LEVEL_MUTE, "UFS: scsi cmd len %d,data len %lu\n", pccb->cmdlen, pccb->datalen);
	printk(LOG_LEVEL_MUTE, "UFS: scsi first 32 byte data");
	for (i = 0; (i < (pccb->datalen)) && (i < 32); i++) { printk(LOG_LEVEL_MUTE, "%02X ", pccb->pdata[i]); }
	printk(LOG_LEVEL_MUTE, "\n");

	printk(LOG_LEVEL_MUTE, "UFS: scsi last 32 byte data");
	if (pccb->datalen > 32) {
		for (i = pccb->datalen - 32; i < pccb->datalen; i++) { printk(LOG_LEVEL_MUTE, "%02X ", pccb->pdata[i]); }
	}
	printk(LOG_LEVEL_MUTE, "\n");
}

static void scsi_setup_inquiry(scsi_cmd_t *pccb) {
	pccb->cmd[0] = SCSI_INQUIRY;
	pccb->cmd[1] = pccb->lun << 5;
	pccb->cmd[2] = 0;
	pccb->cmd[3] = 0;
	if (pccb->datalen > 255)
		pccb->cmd[4] = 255;
	else
		pccb->cmd[4] = (uint8_t) pccb->datalen;
	pccb->cmd[5] = 0;
	pccb->cmdlen = 6;
	pccb->msgout[0] = SCSI_IDENTIFY; /* NOT USED */
}

static void scsi_setup_test_unit_ready(scsi_cmd_t *pccb) {
	pccb->cmd[0] = SCSI_TST_U_RDY;
	pccb->cmd[1] = pccb->lun << 5;
	pccb->cmd[2] = 0;
	pccb->cmd[3] = 0;
	pccb->cmd[4] = 0;
	pccb->cmd[5] = 0;
	pccb->cmdlen = 6;
	pccb->msgout[0] = SCSI_IDENTIFY; /* NOT USED */
}

static void scsi_setup_start(scsi_cmd_t *pccb) {
	pccb->cmd[0] = SCSI_START_STOP;
	pccb->cmd[1] = 1; /* Return immediately */
	pccb->cmd[2] = 0;
	pccb->cmd[3] = 0;
	pccb->cmd[4] = 1; /* Start spin cycle */
	pccb->cmd[5] = 0;
	pccb->cmdlen = 6;
	pccb->msgout[0] = SCSI_IDENTIFY;
}

static int scsi_exec(ufs_device_t *dev, scsi_cmd_t *pccb) {
	return 0;
}

static int scsi_bus_reset(ufs_device_t *dev) {
	return 0;
}

static int scsi_read_capacity(ufs_device_t *dev, scsi_cmd_t *pccb, uint64_t *capacity, uint64_t *blksz) {
	*capacity = 0;

	memset(pccb->cmd, '\0', sizeof(pccb->cmd));
	pccb->cmd[0] = SCSI_RD_CAPAC10;
	pccb->cmd[1] = pccb->lun << 5;
	pccb->cmdlen = 10;
	pccb->msgout[0] = SCSI_IDENTIFY;

	pccb->datalen = 8;
	pccb->dma_dir = DMA_FROM_DEVICE;
	if (scsi_exec(dev, pccb))
		return 1;

	*capacity = ((uint64_t) pccb->pdata[0] << 24) | ((uint64_t) pccb->pdata[1] << 16) | ((uint64_t) pccb->pdata[2] << 8) | ((uint64_t) pccb->pdata[3]);

	if (*capacity != 0xffffffff) {
		*blksz = ((uint64_t) pccb->pdata[4] << 24) | ((uint64_t) pccb->pdata[5] << 16) | ((uint64_t) pccb->pdata[6] << 8) | ((uint64_t) pccb->pdata[7]);
		return 0;
	}

	/* Read capacity (10) was insufficient. Use read capacity (16). */
	memset(pccb->cmd, '\0', sizeof(pccb->cmd));
	pccb->cmd[0] = SCSI_RD_CAPAC16;
	pccb->cmd[1] = 0x10;
	pccb->cmdlen = 16;
	pccb->msgout[0] = SCSI_IDENTIFY; /* NOT USED */

	/*ufs read capacity 16 parameter data is 32 byte*/
	pccb->datalen = 32;
	pccb->dma_dir = DMA_FROM_DEVICE;
	if (scsi_exec(dev, pccb))
		return 1;

	*capacity = ((uint64_t) pccb->pdata[0] << 56) | ((uint64_t) pccb->pdata[1] << 48) | ((uint64_t) pccb->pdata[2] << 40) | ((uint64_t) pccb->pdata[3] << 32) |
				((uint64_t) pccb->pdata[4] << 24) | ((uint64_t) pccb->pdata[5] << 16) | ((uint64_t) pccb->pdata[6] << 8) | ((uint64_t) pccb->pdata[7]);

	/*ufs read capacity 16 parameter data logical block length in bytes is only 4 byte*/
	*blksz = ((uint64_t) pccb->pdata[8] << 24) | ((uint64_t) pccb->pdata[9] << 16) | ((uint64_t) pccb->pdata[10] << 8) | ((uint64_t) pccb->pdata[11]);

	return 0;
}

static int scsi_detect_dev(ufs_device_t *dev, int target, int lun, blk_desc_t *desc) {
	uint8_t perq = 0;
	uint64_t capacity = 0;
	uint64_t blksz = 0;
	int count = 0, err = 0, spintime = 0, try_cnt = 0;
	scsi_cmd_t *pccb = (scsi_cmd_t *) &scsi_cmd_buffer;

	pccb->target = target;
	pccb->lun = lun;
	pccb->pdata = (uint8_t *) &scsi_buffer;
	pccb->datalen = 36;
	pccb->dma_dir = DMA_FROM_DEVICE;
	scsi_setup_inquiry(pccb);

	for (count = 0; count < 3; count++) {
		if (scsi_exec(dev, pccb) != 0) {
			if (pccb->contr_stat == SCSI_SEL_TIME_OUT) {
				printk_warning("UFS: Selection timeout ID %d\n", pccb->target);
				return -1;
			}
			printk_warning("UFS: device not found\n");
			return -1;
		} else {
			printk_infoinfo("UFS: Found UFS device ID %d\n", pccb->target);
			break;
		}
	}
	perq = scsi_buffer[0];
	if ((perq & 0x1f) == 0x1f) {
		printk_warning("UFS: device unknown\n");
		return -1;
	}

	desc->target = pccb->target;
	desc->lun = pccb->lun;

	for (try_cnt = 0; try_cnt < 3; try_cnt++) {
		for (count = 0; count < 3; count++) {
			pccb->datalen = 0;
			pccb->dma_dir = DMA_NONE;
			scsi_setup_test_unit_ready(pccb);
			err = scsi_exec(dev, pccb);
			if (!err)
				break;
		}
		if (err) {
			if (!spintime) {
				spintime = 1;
				pccb->datalen = 0;
				scsi_setup_start(pccb);
				scsi_exec(dev, pccb);
			}
		} else {
			break;
		}
		printk_warning("UFS: try again after start cmd\n");
		mdelay(1000);
	}

	if (err) {
		printk_warning("UFS: try setup device failed\n");
		return -1;
	}

	if (scsi_read_capacity(dev, pccb, &capacity, &blksz)) {
		scsi_print_error(pccb);
		return -1;
	}

	desc->lba = capacity;
	desc->blksz = blksz;
	desc->type = perq;

	return 0;
}

uint64_t scsi_read(ufs_device_t *dev, uint64_t blknr, uint64_t blkcnt, const void *buffer) {
}

uint64_t scsi_write(ufs_device_t *dev, uint64_t blknr, uint64_t blkcnt, const void *buffer) {
}

int scsi_scan_dev(ufs_device_t *dev) {
	ufs_device_t *bdev;
	blk_desc_t bd;
	blk_desc_t *bdesc;

	uint32_t id = 0;
	uint32_t lun = 0;

	/* init dev desc */
	bd.target = 0xff;
	bd.lun = 0xff;

	if (scsi_detect_dev(dev, id, lun, &bd)) {
		printk_warning("UFS: scsi scan device failed\n");
		return -1;
	}

	bdev = dev;
	bdesc = bdev->bd;
	bdesc->target = id;
	bdesc->lun = lun;
	bdesc->type = bd.type;
	bdesc->devnum = 0;
	bdesc->blksz = bd.blksz;
	bdesc->lba = bd.lba;

	printk_info("UFS: detect UFS device\n");
	return 0;
}