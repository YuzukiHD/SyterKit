/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __SYTERKIT_UFS_H__
#define __SYTERKIT_UFS_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <drivers/ufs/ufshc.h>
#include <drivers/ufs/scsi.h>

/**
 * @brief Top-level state for one initialized UFS logical unit.
 */
struct ufs_device {
	/** Host-controller state owned by this device. */
	struct ufshc_host host;
	/** SCSI block-device state for @ref lun. */
	struct ufs_scsi_device scsi;
	/** Logical unit addressed by block operations. */
	uint8_t lun;
	/** True after the host and logical unit have completed initialization. */
	bool initialized;
};

/**
 * @brief Initialize the default UFS logical unit (LUN 0).
 * @param[out] device Device state to initialize.
 * @param[in] config Host-controller base address and timeout configuration.
 * @return Zero on success, otherwise a UFS error code.
 */
int ufs_init(struct ufs_device *device, const struct ufshc_config *config);

/**
 * @brief Initialize a specific UFS logical unit.
 * @param[out] device Device state to initialize.
 * @param[in] config Host-controller configuration.
 * @param[in] lun Logical unit number; values greater than @ref UFS_SCSI_MAX_LUN are invalid.
 * @return Zero on success, otherwise a UFS error code.
 */
int ufs_init_lun(struct ufs_device *device, const struct ufshc_config *config, uint8_t lun);

/**
 * @brief Stop the host controller and release the device state.
 */
void ufs_exit(struct ufs_device *device);

/**
 * @brief Read blocks from the selected logical unit.
 * @param[in] device Initialized UFS device.
 * @param[in] lba First logical block address.
 * @param[in] blocks Number of blocks to read.
 * @param[out] buffer Destination buffer, sized for @p blocks blocks.
 * @return Zero on success, otherwise a UFS error code.
 */
int ufs_read(struct ufs_device *device, uint64_t lba, uint32_t blocks, void *buffer);

/**
 * @brief Write blocks to the selected logical unit.
 * @param[in] device Initialized UFS device.
 * @param[in] lba First logical block address.
 * @param[in] blocks Number of blocks to write.
 * @param[in] buffer Source buffer, sized for @p blocks blocks.
 * @return Zero on success, otherwise a UFS error code.
 */
int ufs_write(struct ufs_device *device, uint64_t lba, uint32_t blocks, const void *buffer);

/**
 * @brief Return the number of addressable blocks on the selected LUN.
 */
uint64_t ufs_capacity(const struct ufs_device *device);

/**
 * @brief Return the logical block size in bytes.
 */
uint32_t ufs_block_size(const struct ufs_device *device);

/**
 * @brief Return the manufacturer identifier reported by the device.
 */
uint16_t ufs_manufacturer_id(const struct ufs_device *device);

/* Block-oriented wrappers mirror the MMC driver's early-boot API. */
/**
 * @brief Read blocks using the MMC-compatible block-layer convention.
 * @return Number of blocks transferred, or zero on failure.
 */
uint32_t ufs_blk_read(struct ufs_device *device, void *buffer, uint64_t lba, uint32_t blocks);

/**
 * @brief Write blocks using the MMC-compatible block-layer convention.
 * @return Number of blocks transferred, or zero on failure.
 */
uint32_t ufs_blk_write(struct ufs_device *device, const void *buffer, uint64_t lba, uint32_t blocks);

#endif /* __SYTERKIT_UFS_H__ */
