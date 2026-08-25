/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __SYTERKIT_UFS_SCSI_H__
#define __SYTERKIT_UFS_SCSI_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct ufshc_host;

#define UFS_SCSI_READ10		 0x28U
#define UFS_SCSI_WRITE10	 0x2aU
#define UFS_SCSI_READ16		 0x88U
#define UFS_SCSI_WRITE16	 0x8aU
#define UFS_SCSI_INQUIRY	 0x12U
#define UFS_SCSI_READ_CAPACITY10 0x25U
#define UFS_SCSI_READ_CAPACITY16 0x9eU
#define UFS_SCSI_TEST_UNIT_READY 0x00U
#define UFS_SCSI_START_STOP	 0x1bU
#define UFS_SCSI_REQUEST_SENSE	 0x03U

#define UFS_SCSI_GOOD		     0x00U
#define UFS_SCSI_CHECK_CONDITION     0x02U
#define UFS_SCSI_MAX_BLOCKS	     128U
#define UFS_SCSI_MAX_LUN	     0x7fU
#define UFS_SCSI_READ16_THRESHOLD    0x0fffffffULL
#define UFS_SCSI_INQUIRY_DATA_LENGTH 36U

struct ufs_scsi_device {
	struct ufshc_host *host;
	uint8_t lun;
	uint32_t block_size;
	uint64_t block_count;
	uint16_t manufacturer_id;
	char model[17];
	uint8_t last_response_type;
	uint8_t last_task_response;
	uint8_t last_status;
	uint8_t last_sense_length;
	uint8_t last_sense[18];
	uint32_t last_residual_transfer_count;
	bool present;
};

int ufs_scsi_init(struct ufs_scsi_device *device, struct ufshc_host *host, uint8_t lun);
int ufs_scsi_exec(
	struct ufs_scsi_device *device, const uint8_t *cdb, uint8_t cdb_len, void *data, size_t data_len, bool write);
int ufs_scsi_test_unit_ready(struct ufs_scsi_device *device);
int ufs_scsi_request_sense(struct ufs_scsi_device *device);
int ufs_scsi_read(struct ufs_scsi_device *device, uint64_t lba, uint32_t blocks, void *buffer);
int ufs_scsi_write(struct ufs_scsi_device *device, uint64_t lba, uint32_t blocks, const void *buffer);

#endif /* __SYTERKIT_UFS_SCSI_H__ */
