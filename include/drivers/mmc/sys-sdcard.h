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

/**
 * Initialize the SDMMC controller with the specified platform data and SDHCI driver.
 *
 * @param data Pointer to the SDMMC platform data structure.
 * @param hci Pointer to the SDHCI driver instance.
 * @return 0 if successful, or an error code if failed.
 */
int sdmmc_init(sdmmc_pdata_t *data, sunxi_sdhci_t *hci);

/**
 * Read data from the SDMMC card into the provided buffer.
 *
 * @param data Pointer to the SDMMC platform data structure.
 * @param buf Pointer to the destination buffer to store the read data.
 * @param blkno The starting block number to read from.
 * @param blkcnt The number of blocks to read.
 * @return The total number of bytes read, or an error code if failed.
 */
uint32_t sdmmc_blk_read(sdmmc_pdata_t *data, uint8_t *buf, uint32_t blkno, uint32_t blkcnt);

#ifdef __cplusplus
}
#endif// __cplusplus

#endif// _SYS_SDCARD_H_