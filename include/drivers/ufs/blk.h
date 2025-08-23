/* SPDX-License-Identifier:	GPL-2.0+ */

#ifndef __BLK_H__
#define __BLK_H__

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

typedef struct blk_desc {
	int devnum;		   /* device number */
	uint8_t part_type; /* partition type */
	uint8_t target;	   /* target SCSI ID */
	uint8_t lun;	   /* target LUN */
	uint8_t hwpart;	   /* HW partition, e.g. for eMMC */
	uint8_t type;	   /* device type */
	uint8_t removable; /* removable device */
	uint64_t lba;	   /* number of blocks */
	uint64_t blksz;	   /* block size */
} blk_desc_t;

#endif// __BLK_H__