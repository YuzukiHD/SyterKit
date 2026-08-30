/* SPDX-License-Identifier:	GPL-2.0+ */
#define pr_fmt(fmt) "mmc-sun55iw6: " fmt

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>
#include <timer.h>

#include <drivers/gpio/gpio.h>

#include <drivers/mmc/mmc.h>
#include <drivers/mmc/sdhci.h>

/**
 * @brief Set the SDHC controller's clock frequency.
 * 
 * This function sets the clock frequency for the specified SDHC controller.
 * 
 * @param sdhci Pointer to the SDHC controller structure.
 * @param clk_hz Desired clock frequency in Hertz.
 * @return Returns 0 on success, -1 on failure.
 */
int sunxi_sdhci_set_mclk(sunxi_sdhci_t *sdhci, uint32_t clk_hz)
{
	uint32_t reg_val = 0x0;
	uint32_t parent_rate;
	uint32_t source;

	if (sdhci == NULL || clk_hz == 0U)
		return -1;
	mmc_t *mmc = &sdhci->mmc;
	sunxi_sdhci_clk_t clk = sdhci->sdhci_clk;

	// Determine the clock source based on the requested frequency
	if (clk_hz <= 4000000) {
		source = 0U;
	} else {
		source = clk.default_clk_sel;
	}

	if ((mmc->speed_mode == MMC_HSDDR52_DDR50) && (mmc->bus_width == SMHC_WIDTH_8BIT)) {
		clk_hz /= 4;
	} else {
		clk_hz /= 2;
	}

	// Set the clock divider values based on the requested frequency
	switch (clk_hz) {
	case 0 ... 400000:
		clk.factor_n = 1;
		clk.factor_m = 0xe;
		break;
	case 400001 ... 26000000:
		clk.factor_n = (sdhci->id == 2) ? 2 : 1;
		clk.factor_m = (sdhci->id == 2) ? 3 : 2;
		break;
	case 26000001 ... 52000000:
		clk.factor_n = (sdhci->id == 2) ? 1 : 0;
		clk.factor_m = 2;
		break;
	case 52000001 ... 100000000:
		source = 1U;
		clk.factor_n = 0;
		clk.factor_m = 3;
		break;
	case 100000001 ... 150000000:
		source = 1U;
		clk.factor_n = 0;
		clk.factor_m = 2;
		break;
	case 150000001 ... 200000000:
		source = 1U;
		clk.factor_n = 0;
		clk.factor_m = 1;
		break;
	default:
		clk.factor_n = (sdhci->id == 2) ? 1 : 0;
		clk.factor_m = 2;
		pr_debug("requested frequency does not match: freq=%d, default to 52000000\n", clk_hz);
		break;
	}
	parent_rate = sunxi_sdhci_clk_source_rate(&clk, source);
	if (parent_rate == 0U) {
		pr_debug("unsupported clock source %u\n", source);
		return -1;
	}

	// Configure the clock register value
	reg_val = (source << 24) | (clk.factor_n << clk.reg_factor_n_offset) | (clk.factor_m << clk.reg_factor_m_offset);
	writel(reg_val, clk.reg_base);

	return 0;
}

/**
 * @brief Apply the fixed MMC2 data-line deskew on SUN55IW6.
 *
 * SUN55IW6's eMMC controller (MMC2) needs a fixed deskew delay on all
 * eight data lines regardless of tuning, so program it once when the
 * controller enters HS400 timing.
 *
 * @param sdhci Pointer to the SDHC controller structure.
 * @return 0 on success, or a negative value on failure.
 */
int sunxi_sdhci_set_skew(sunxi_sdhci_t *sdhci)
{
	sunxi_sdhci_host_t *mmc_host;

	if (sdhci == NULL)
		return -1;
	if (sdhci->id != MMC_CONTROLLER_2)
		return 0;

	mmc_host = &sdhci->mmc_host;
	mmc_host->reg->skew_dat0_dl = 0x8f;
	mmc_host->reg->skew_dat1_dl = 0x8f;
	mmc_host->reg->skew_dat2_dl = 0x8f;
	mmc_host->reg->skew_dat3_dl = 0x8f;
	mmc_host->reg->skew_dat4_dl = 0x8f;
	mmc_host->reg->skew_dat5_dl = 0x8f;
	mmc_host->reg->skew_dat6_dl = 0x8f;
	mmc_host->reg->skew_dat7_dl = 0x8f;
	mmc_host->reg->skew_ctrl = 0xc;

	return 0;
}
