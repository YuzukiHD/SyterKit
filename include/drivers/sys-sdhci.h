/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __SDHCI_H__
#define __SDHCI_H__

#ifdef CONFIG_CHIP_MMC_V2
#include <mmc/sys-sdhci.h>
#else
#include <sdhci/sys-sdhci.h>
#endif /* CONFIG_CHIP_MMC_V2 */

#endif /* __SDHCI_H__ */
