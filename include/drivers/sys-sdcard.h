/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __SDCARD_H__
#define __SDCARD_H__

#ifdef CONFIG_CHIP_MMC_V2
#include <mmc/sys-sdcard.h>
#else
#include <sdhci/sys-sdcard.h>
#endif /* CONFIG_CHIP_MMC_V2 */

#endif /* __SDCARD_H__ */
