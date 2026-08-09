/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __SDHCI_H__
#define __SDHCI_H__

#ifdef CONFIG_DRIVER_MMC_V2
#include <drivers/mmc/sdhci.h>
#else
#include <drivers/sdhci/sdhci.h>
#endif /* CONFIG_DRIVER_MMC_V2 */

#endif /* __SDHCI_H__ */
