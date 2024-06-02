/* SPDX-License-Identifier:	GPL-2.0+ */
/* MMC driver for General mmc operations 
 * Original https://github.com/smaeul/sun20i_d1_spl/blob/mainline/drivers/mmc/sun20iw1p1/
 */

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