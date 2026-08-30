/* SPDX-License-Identifier: GPL-2.0+ */
#define pr_fmt(fmt) "SMHC: " fmt

#include <stdint.h>

#include <log.h>

#include <drivers/mmc/hs-timing.h>
#include <drivers/mmc/tuning.h>

void sunxi_mmc_hs400_mode_set(sunxi_sdhci_t *sdhci, bool status)
{
	uint32_t dsbd;
	uint32_t csdc;

	if (sdhci == NULL || sdhci->id != MMC_CONTROLLER_2 ||
	    sdhci->mmc_host.reg == NULL)
		return;

	dsbd = sdhci->mmc_host.reg->dsbd;
	csdc = sdhci->mmc_host.reg->csdc;

	dsbd &= ~BIT(31);
	csdc &= ~0xfU;
	if (status) {
		dsbd |= BIT(31);
		csdc |= 0x6U;
	} else {
		csdc |= 0x3U;
	}

	sdhci->mmc_host.reg->dsbd = dsbd;
	sdhci->mmc_host.reg->csdc = csdc;
	pr_trace("HS400 mode %s\n", status ? "enabled" : "disabled");
}

/**
 * @brief Switch the Sunxi SDHCI controller to High Speed 200 (HS200) mode.
 *
 * @param sdhci A pointer to the Sunxi SDHCI controller structure.
 * @return Returns 0 on success, or a negative error code on failure.
 */
int sunxi_mmc_mmc_switch_hs200(sunxi_sdhci_t *sdhci)
{
	if (sdhci == NULL || sdhci->id != MMC_CONTROLLER_2 ||
	    sdhci->mmc_host.timing_mode != SUNXI_MMC_TIMING_MODE_4)
		return -1;

	mmc_t *mmc = &sdhci->mmc;
	int err;

	if (mmc->speed_mode == MMC_HS200_SDR104) {
		pr_trace("set in SDR104 mode\n");
		return 0;
	}

	if (!(mmc->card_caps & MMC_MODE_HS200)) {
		pr_warn("Card does not support HS200 mode\n");
		return -1;
	}

	err = sunxi_mmc_hs_switch_card(sdhci, EXT_CSD_CMD_SET_NORMAL,
				EXT_CSD_HS_TIMING, EXT_CSD_TIMING_HS200);
	if (err) {
		pr_warn("Failed to change to HS200 mode\n");
		return err;
	}

	mmc->speed_mode = MMC_HS200_SDR104;
	sunxi_mmc_hs_set_clock(sdhci, mmc->clock);
	if (sdhci->mmc_host.fatal_err)
		return -1;

	err = sunxi_mmc_hs_wait_status(sdhci);
	if (err)
		return err;

	return 0;
}

/**
 * @brief Switch the Sunxi SDHCI controller to High Speed 400 (HS400) mode.
 *
 * @param sdhci A pointer to the Sunxi SDHCI controller structure.
 * @return Returns 0 on success, or a negative error code on failure.
 */
int sunxi_mmc_mmc_switch_hs400(sunxi_sdhci_t *sdhci)
{
	if (sdhci == NULL || sdhci->id != MMC_CONTROLLER_2 ||
	    sdhci->mmc_host.timing_mode != SUNXI_MMC_TIMING_MODE_4)
		return -1;

	mmc_t *mmc = &sdhci->mmc;
	int err;

	if (mmc->speed_mode == MMC_HS400) {
		pr_trace("set in HS400 mode\n");
		return 0;
	}

	if (!(mmc->card_caps & MMC_MODE_HS400)) {
		pr_warn("Card does not support HS400 mode\n");
		return -1;
	}

	err = sunxi_mmc_hs_switch_card(sdhci, EXT_CSD_CMD_SET_NORMAL,
				EXT_CSD_HS_TIMING, EXT_CSD_TIMING_HS400);
	if (err) {
		pr_warn("Failed to change to HS400 mode\n");
		return err;
	}

	mmc->speed_mode = MMC_HS400;
	sunxi_mmc_hs_set_clock(sdhci, mmc->clock);
	if (sdhci->mmc_host.fatal_err)
		return -1;

	err = sunxi_mmc_hs_wait_status(sdhci);
	if (err)
		return err;

	return 0;
}

int sunxi_mmc_mmc_prepare_hs200(sunxi_sdhci_t *sdhci, uint32_t width)
{
	if (sdhci == NULL || sdhci->id != MMC_CONTROLLER_2 ||
	    sdhci->mmc_host.timing_mode != SUNXI_MMC_TIMING_MODE_4 ||
	    (width != SMHC_WIDTH_4BIT && width != SMHC_WIDTH_8BIT))
		return -1;

	mmc_t *mmc = &sdhci->mmc;
	int err;

	err = sunxi_mmc_hs_switch_bus_mode(sdhci, MMC_HS200_SDR104, width);
	if (err)
		return err;

	/* A board may advertise a high maximum clock that has no usable timing
	 * window. Keep the card in HS200 and retry at lower standard points. */
	static const uint32_t tune_clocks[] = { 200000000U, 150000000U, 100000000U, 50000000U };
	uint32_t previous = 0U;
	int last_err = -1;

	for (size_t index = 0U; index < sizeof(tune_clocks) / sizeof(tune_clocks[0]); ++index) {
		uint32_t candidate = mmc->f_max < tune_clocks[index] ? mmc->f_max : tune_clocks[index];

		if (candidate < mmc->f_min || candidate == previous)
			continue;
		previous = candidate;
		sunxi_mmc_hs_set_clock(sdhci, candidate);
		if (sdhci->mmc_host.fatal_err)
			return -1;

		last_err = sunxi_mmc_execute_tuning(sdhci);
		if (!last_err) {
			mmc->tran_speed = mmc->clock;
			pr_info("speed mode: HS200/SDR104\n");
			pr_info("HS200/SDR104: 0x%08x 0x%08x\n",
				mmc->tune_sdly.tm4_smx_fx[MMC_HS200_SDR104 * 2U],
				mmc->tune_sdly.tm4_smx_fx[MMC_HS200_SDR104 * 2U + 1U]);
			return 0;
		}
	}

	return last_err;
}

int sunxi_mmc_mmc_prepare_hs400(sunxi_sdhci_t *sdhci, uint32_t width)
{
	if (sdhci == NULL || sdhci->id != MMC_CONTROLLER_2 ||
	    sdhci->mmc_host.timing_mode != SUNXI_MMC_TIMING_MODE_4)
		return -1;

	mmc_t *mmc = &sdhci->mmc;
	uint32_t clock = mmc->f_max;
	int err;

	if (width != SMHC_WIDTH_8BIT)
		return -1;

	err = sunxi_mmc_mmc_prepare_hs200(sdhci, width);
	if (err)
		return err;

	clock = mmc->clock;
	if (clock > 200000000U)
		clock = 200000000U;

	/* Return to a safe HS clock before switching the card to 8-bit DDR. */
	sunxi_mmc_hs_set_clock(sdhci, 52000000U);
	if (sdhci->mmc_host.fatal_err)
		return -1;

	/* The card is still in HS200. Leave HS200 while the host keeps the
	 * HS200 protocol, then reconfigure the host for HS before switching
	 * the card's bus width to DDR. */
	err = sunxi_mmc_hs_switch_card(sdhci, EXT_CSD_CMD_SET_NORMAL,
				EXT_CSD_HS_TIMING, EXT_CSD_TIMING_HS);
	if (err)
		return err;

	mmc->speed_mode = MMC_HSSDR52_SDR25;
	sunxi_mmc_hs_set_clock(sdhci, 52000000U);
	if (sdhci->mmc_host.fatal_err)
		return -1;
	err = sunxi_mmc_hs_wait_status(sdhci);
	if (err)
		return err;

	err = sunxi_mmc_hs_switch_bus_mode(sdhci, MMC_HSDDR52_DDR50, width);
	if (err)
		return err;

	pr_info("================== HS400...\n");
	err = sunxi_mmc_mmc_switch_hs400(sdhci);
	if (err)
		return err;

	mmc->tran_speed = clock;
	sunxi_mmc_hs_set_clock(sdhci, clock);
	if (sdhci->mmc_host.fatal_err)
		return -1;

	err = sunxi_mmc_execute_hs400_command_tuning(sdhci);
	if (err)
		return err;

	pr_info("speed mode: HS400\n");
	err = sunxi_mmc_execute_hs400_tuning(sdhci);
	if (err)
		return err;

	pr_info("HS400: 0x%08x 0x%08x\n",
		(uint32_t)mmc->tune_sdly.tm4_dsdly[0] |
		((uint32_t)mmc->tune_sdly.tm4_dsdly[1] << 8) |
		((uint32_t)mmc->tune_sdly.tm4_dsdly[2] << 16) |
		((uint32_t)mmc->tune_sdly.tm4_dsdly[3] << 24),
		(uint32_t)mmc->tune_sdly.tm4_dsdly[4] |
		((uint32_t)mmc->tune_sdly.tm4_dsdly[5] << 8) |
		((uint32_t)mmc->tune_sdly.tm4_dsdly[6] << 16) |
		((uint32_t)mmc->tune_sdly.tm4_dsdly[7] << 24));
	pr_info("HS400: 0x%08x 0x%08x\n",
		mmc->tune_sdly.tm4_smx_fx[MMC_HS400 * 2U],
		mmc->tune_sdly.tm4_smx_fx[MMC_HS400 * 2U + 1U]);

	return 0;
}

int sunxi_mmc_mmc_downgrade_high_speed(sunxi_sdhci_t *sdhci)
{
	if (sdhci == NULL)
		return -1;

	mmc_t *mmc = &sdhci->mmc;
	int err;

	uint8_t bus_width;

	if (mmc->bus_width == SMHC_WIDTH_8BIT)
		bus_width = EXT_CSD_BUS_WIDTH_8;
	else if (mmc->bus_width == SMHC_WIDTH_4BIT)
		bus_width = EXT_CSD_BUS_WIDTH_4;
	else if (mmc->bus_width == SMHC_WIDTH_1BIT)
		bus_width = EXT_CSD_BUS_WIDTH_1;
	else
		return -1;

	/* A stale DDR state with a one-bit bus can only be left by a failed
	 * transition before the card-side DDR switch completed. Recover it as
	 * the identification-time SDR state first. */
	if (mmc->speed_mode == MMC_HSDDR52_DDR50 &&
	    mmc->bus_width == SMHC_WIDTH_1BIT)
		mmc->speed_mode = MMC_HSSDR52_SDR25;

	/* HS400 must be exited while the host still speaks HS400. Change the
	 * card back to HS, switch the host to HS-DDR, then change the card to
	 * SDR bus width before returning to the normal SDR state. */
	if (mmc->speed_mode == MMC_HS400) {
		sunxi_mmc_hs_set_clock(sdhci, 52000000U);
		if (sdhci->mmc_host.fatal_err)
			return -1;

		err = sunxi_mmc_hs_switch_card(sdhci, EXT_CSD_CMD_SET_NORMAL,
					EXT_CSD_HS_TIMING, EXT_CSD_TIMING_HS);
		if (err)
			return err;

		mmc->speed_mode = MMC_HSDDR52_DDR50;
		sunxi_mmc_hs_set_clock(sdhci, 52000000U);
		if (sdhci->mmc_host.fatal_err)
			return -1;
		err = sunxi_mmc_hs_wait_status(sdhci);
		if (err)
			return err;

		err = sunxi_mmc_hs_switch_card(sdhci, EXT_CSD_CMD_SET_NORMAL,
					EXT_CSD_BUS_WIDTH, bus_width);
		if (err)
			return err;

		mmc->speed_mode = MMC_HSSDR52_SDR25;
		sunxi_mmc_hs_set_bus_width(sdhci, mmc->bus_width);
		if (sdhci->mmc_host.fatal_err)
			return -1;
		return sunxi_mmc_hs_wait_status(sdhci);
	}

	if (mmc->speed_mode == MMC_HSDDR52_DDR50) {
		/* HS400 preparation already changed the card to HS and DDR. */
		sunxi_mmc_hs_set_clock(sdhci, 52000000U);
		if (sdhci->mmc_host.fatal_err)
			return -1;

		err = sunxi_mmc_hs_switch_card(sdhci, EXT_CSD_CMD_SET_NORMAL,
					EXT_CSD_BUS_WIDTH, bus_width);
		if (err)
			return err;

		mmc->speed_mode = MMC_HSSDR52_SDR25;
		sunxi_mmc_hs_set_bus_width(sdhci, mmc->bus_width);
		sunxi_mmc_hs_set_clock(sdhci, 52000000U);
		if (sdhci->mmc_host.fatal_err)
			return -1;
		return sunxi_mmc_hs_wait_status(sdhci);
	}

	sunxi_mmc_hs_set_clock(sdhci, 52000000U);
	if (sdhci->mmc_host.fatal_err)
		return -1;

	err = sunxi_mmc_hs_switch_card(sdhci, EXT_CSD_CMD_SET_NORMAL,
				EXT_CSD_HS_TIMING, EXT_CSD_TIMING_HS);
	if (err)
		pr_warn("failed to downgrade after tuning\n");
	else {
		mmc->speed_mode = MMC_HSSDR52_SDR25;
		sunxi_mmc_hs_set_clock(sdhci, 52000000U);
		if (sdhci->mmc_host.fatal_err)
			return -1;
		err = sunxi_mmc_hs_wait_status(sdhci);
		if (err)
			return err;
	}
	return err;
}
