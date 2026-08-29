/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef _SYS_MMC_HS_TIMING_H_
#define _SYS_MMC_HS_TIMING_H_

#include <drivers/mmc/sdhci.h>

#if CONFIG_DRIVER_MMC_TUNING
int sunxi_mmc_hs_switch_card(sunxi_sdhci_t *sdhci, uint8_t set,
				      uint8_t index, uint8_t value);
int sunxi_mmc_hs_wait_status(sunxi_sdhci_t *sdhci);
void sunxi_mmc_hs_set_clock(sunxi_sdhci_t *sdhci, uint32_t clock);
void sunxi_mmc_hs_set_bus_width(sunxi_sdhci_t *sdhci, uint32_t width);
int sunxi_mmc_hs_switch_bus_mode(sunxi_sdhci_t *sdhci, uint32_t spd_mode,
				  uint32_t width);
int sunxi_mmc_mmc_switch_hs200(sunxi_sdhci_t *sdhci);
int sunxi_mmc_mmc_prepare_hs200(sunxi_sdhci_t *sdhci, uint32_t width);
int sunxi_mmc_mmc_downgrade_high_speed(sunxi_sdhci_t *sdhci);
void sunxi_mmc_hs400_mode_set(sunxi_sdhci_t *sdhci, bool status);
int sunxi_mmc_mmc_switch_hs400(sunxi_sdhci_t *sdhci);
int sunxi_mmc_mmc_prepare_hs400(sunxi_sdhci_t *sdhci, uint32_t width);
#endif

#endif /* _SYS_MMC_HS_TIMING_H_ */
