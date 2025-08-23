/* SPDX-License-Identifier:	GPL-2.0+ */

#ifndef _SYS_SDCARD_H_
#define _SYS_SDCARD_H_

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include "sys-mmc.h"
#include "sys-sdhci.h"

#ifdef __cplusplus
extern "C" {
#endif// __cplusplus

typedef struct {
	sunxi_sdhci_t *hci;
	bool online;
} sdmmc_pdata_t;

/**
 * External declaration of the 'card0' SDMMC platform data structure.
 */
extern sdmmc_pdata_t card0;
extern sdmmc_pdata_t card2;

/**
 * @brief Initialize the SD/MMC interface
 *
 * Initializes the SD/MMC interface using the provided data structure and SD Host Controller instance.
 * It sets the SD Host Controller instance and the online status in the data structure, and then initializes
 * the MMC device. If the initialization is successful, it prints a message indicating the type of card detected
 * and returns 0; otherwise, it returns -1.
 *
 * @param data  Pointer to the SD/MMC platform data structure
 * @param hci   Pointer to the Sunxi SD Host Controller instance
 *
 * @return      Returns 0 on success, or -1 if the initialization fails
 */
int sdmmc_init(sdmmc_pdata_t *data, sunxi_sdhci_t *hci);

/**
 * @brief Read blocks from the SD/MMC device
 *
 * Reads a specified number of blocks from the SD/MMC device using the provided platform data structure,
 * destination buffer, block number, and block count. It calls the underlying function for reading blocks
 * from the Sunxi MMC block device and returns the result.
 *
 * @param data      Pointer to the SD/MMC platform data structure
 * @param buf       Pointer to the destination buffer where the read data will be stored
 * @param blkno     The starting block number to read from
 * @param blkcnt    The number of blocks to read
 *
 * @return          Returns 0 on success, or an error code if the operation fails
 */
uint32_t sdmmc_blk_read(sdmmc_pdata_t *data, uint8_t *buf, uint32_t blkno, uint32_t blkcnt);

/**
 * @brief Writes blocks of data to the SD/MMC device using the specified SDHCI instance.
 *
 * This function writes a specified number of blocks to the SD/MMC (Secure Digital/MultiMediaCard) device
 * associated with the given SDHCI (SD Host Controller Interface) instance. It serves as a wrapper around
 * the sunxi_mmc_blk_write function, providing a simplified interface for block data write operations.
 *
 * @param[in] data A pointer to the SD/MMC platform data structure.
 * @param[in] buf A pointer to the source buffer from which data will be written to the SD/MMC device.
 * @param[in] blkno The starting block number where the data writing begins.
 * @param[in] blkcnt The number of blocks to write.
 *
 * @return The number of blocks successfully written, or 0 if writing failed.
 */
uint32_t sdmmc_blk_write(sdmmc_pdata_t *data, uint8_t *buf, uint32_t blkno, uint32_t blkcnt);

#ifdef __cplusplus
}
#endif// __cplusplus

#endif// _SYS_SDCARD_H_