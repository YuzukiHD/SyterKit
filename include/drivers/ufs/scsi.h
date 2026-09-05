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

/**
 * @brief SCSI discovery and transfer state for one UFS logical unit.
 */
struct ufs_scsi_device {
	/** Host-controller used to submit UPIUs. */
	struct ufshc_host *host;
	/** Logical unit number. */
	uint8_t lun;
	/** Logical block size in bytes reported by READ CAPACITY. */
	uint32_t block_size;
	/** Number of addressable logical blocks. */
	uint64_t block_count;
	/** Device manufacturer identifier from the descriptor. */
	uint16_t manufacturer_id;
	/** NUL-terminated product model, when reported by INQUIRY. */
	char model[17];
	/** Last response transaction type. */
	uint8_t last_response_type;
	/** Last UPIU task response code. */
	uint8_t last_task_response;
	/** Last SCSI status byte. */
	uint8_t last_status;
	/** Number of valid bytes in @ref last_sense. */
	uint8_t last_sense_length;
	/** Sense data returned by the last failed command. */
	uint8_t last_sense[18];
	/** Bytes not transferred by the last command. */
	uint32_t last_residual_transfer_count;
	/** True when the logical unit passed discovery and readiness checks. */
	bool present;
};

/**
 * @brief Discover and initialize a SCSI logical unit.
 * @param[out] device SCSI state to initialize.
 * @param[in] host Initialized UFS host controller.
 * @param[in] lun Logical unit number to discover.
 * @return Zero on success, otherwise a UFS error code.
 */
int ufs_scsi_init(struct ufs_scsi_device *device, struct ufshc_host *host, uint8_t lun);

/**
 * @brief Submit an arbitrary SCSI CDB through the UFS host controller.
 * @param[in,out] device SCSI device used for the request.
 * @param[in] cdb SCSI command descriptor block.
 * @param[in] cdb_len Number of valid bytes in @p cdb.
 * @param[in,out] data Optional data buffer.
 * @param[in] data_len Data buffer length in bytes.
 * @param[in] write True for a host-to-device transfer.
 * @return Zero on a successful command and status, otherwise a UFS error code.
 */
int ufs_scsi_exec(
	struct ufs_scsi_device *device, const uint8_t *cdb, uint8_t cdb_len, void *data, size_t data_len, bool write);

/**
 * @brief Verify that the logical unit is ready for commands.
 * @param[in] device Initialized SCSI device.
 * @return Zero when ready, otherwise a UFS error code.
 */
int ufs_scsi_test_unit_ready(struct ufs_scsi_device *device);

/**
 * @brief Request and cache sense data for the last failed command.
 * @param[in,out] device SCSI device whose sense data is updated.
 * @return Zero on success, otherwise a UFS error code.
 */
int ufs_scsi_request_sense(struct ufs_scsi_device *device);

/**
 * @brief Read up to the implementation's maximum SCSI transfer size.
 * @param[in] device Initialized SCSI device.
 * @param[in] lba First logical block address.
 * @param[in] blocks Number of blocks to read.
 * @param[out] buffer Destination buffer.
 * @return Zero on success, otherwise a UFS error code.
 */
int ufs_scsi_read(struct ufs_scsi_device *device, uint64_t lba, uint32_t blocks, void *buffer);

/**
 * @brief Write up to the implementation's maximum SCSI transfer size.
 * @param[in] device Initialized SCSI device.
 * @param[in] lba First logical block address.
 * @param[in] blocks Number of blocks to write.
 * @param[in] buffer Source buffer.
 * @return Zero on success, otherwise a UFS error code.
 */
int ufs_scsi_write(struct ufs_scsi_device *device, uint64_t lba, uint32_t blocks, const void *buffer);

#endif /* __SYTERKIT_UFS_SCSI_H__ */
