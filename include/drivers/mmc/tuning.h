/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef _SYS_MMC_TUNING_H_
#define _SYS_MMC_TUNING_H_

#include <drivers/mmc/sdhci.h>

#if CONFIG_DRIVER_MMC_TUNING
int sunxi_mmc_execute_tuning(sunxi_sdhci_t *sdhci);
int sunxi_mmc_execute_hs400_command_tuning(sunxi_sdhci_t *sdhci);
int sunxi_mmc_execute_hs400_tuning(sunxi_sdhci_t *sdhci);
#endif

#endif /* _SYS_MMC_TUNING_H_ */
