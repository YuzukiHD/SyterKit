/* SPDX-License-Identifier:	GPL-2.0+ */

#include <barrier.h>
#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>
#include <timer.h>

#include <sys-clk.h>
#include <sys-gpio.h>

#include <mmc/sys-sdcard.h>

sdmmc_pdata_t card0;

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
int sdmmc_init(sdmmc_pdata_t *data, sunxi_sdhci_t *hci) {
    data->hci = hci;
    data->online = false;

    if (sunxi_mmc_init(data->hci) == 0) {
        printk_info("SHMC: %s card detected\n", data->hci->sdhci_mmc_type == MMC_TYPE_SD ? "SD" : "MMC");
        return 0;
    }

    return -1;
}

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
uint32_t sdmmc_blk_read(sdmmc_pdata_t *data, uint8_t *buf, uint32_t blkno, uint32_t blkcnt) {
    return sunxi_mmc_blk_read(data->hci, buf, blkno, blkcnt);
}



uint32_t sdmmc_blk_write(sdmmc_pdata_t *data, uint8_t *buf, uint32_t blkno, uint32_t blkcnt) {
    return sunxi_mmc_blk_write(data->hci, buf, blkno, blkcnt);
}
