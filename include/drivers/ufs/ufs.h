/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __SYTERKIT_UFS_H__
#define __SYTERKIT_UFS_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <drivers/ufs/ufshc.h>
#include <drivers/ufs/scsi.h>

struct ufs_device {
	struct ufshc_host host;
	struct ufs_scsi_device scsi;
	uint8_t lun;
	bool initialized;
};

int ufs_init(struct ufs_device *device, const struct ufshc_config *config);
int ufs_init_lun(struct ufs_device *device, const struct ufshc_config *config, uint8_t lun);
void ufs_exit(struct ufs_device *device);
int ufs_read(struct ufs_device *device, uint64_t lba, uint32_t blocks, void *buffer);
int ufs_write(struct ufs_device *device, uint64_t lba, uint32_t blocks, const void *buffer);
uint64_t ufs_capacity(const struct ufs_device *device);
uint32_t ufs_block_size(const struct ufs_device *device);
uint16_t ufs_manufacturer_id(const struct ufs_device *device);

/* Block-oriented wrappers mirror the MMC driver's early-boot API. */
uint32_t ufs_blk_read(struct ufs_device *device, void *buffer, uint64_t lba, uint32_t blocks);
uint32_t ufs_blk_write(struct ufs_device *device, const void *buffer, uint64_t lba, uint32_t blocks);

#endif /* __SYTERKIT_UFS_H__ */
