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

int sdmmc_init(sdmmc_pdata_t *data, sunxi_sdhci_t *hci) {
    data->hci = hci;
    data->online = false;

    if (sunxi_mmc_init(data->hci) == 0) {
        printk_info("SHMC: %s card detected\n", data->hci->sdhci_mmc_type & MMC_TYPE_SD ? "SD" : "MMC");
        return 0;
    }

    return -1;
}

uint32_t sdmmc_blk_read(sdmmc_pdata_t *data, uint8_t *buf, uint32_t blkno, uint32_t blkcnt) {
    return sunxi_mmc_blk_read(data->hci, buf, blkno, blkcnt);
}