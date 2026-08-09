/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __SDCARD_H__
#define __SDCARD_H__

#ifdef CONFIG_DRIVER_MMC_V2
#include <drivers/mmc/sdcard.h>
#else
#include <drivers/sdhci/sdcard.h>
#endif /* CONFIG_DRIVER_MMC_V2 */

#endif /* __SDCARD_H__ */
