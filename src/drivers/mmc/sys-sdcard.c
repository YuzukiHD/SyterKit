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
    return 0;
}

uint64_t sdmmc_blk_read(sdmmc_pdata_t *data, uint8_t *buf, uint64_t blkno, uint64_t blkcnt) {
    return 0;
}