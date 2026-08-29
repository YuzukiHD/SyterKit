/* SPDX-License-Identifier: GPL-2.0+ */
#define pr_fmt(fmt) "SMHC: " fmt

#include <stdint.h>

#include <log.h>
#include <string.h>

#include <drivers/mmc/mmc.h>
#include <drivers/mmc/sdhci.h>
#include <drivers/mmc/tuning.h>

#define SUNXI_MMC_TUNING_POINTS 64U
#define SUNXI_MMC_TUNING_MIN_WINDOW 12U
#define SUNXI_MMC_TUNING_INVALID 0xffffffffU
#define SUNXI_MMC_TUNING_BLOCK_SIZE 512U
/* A tuning probe at 50 MHz or above completes well within this interval. */
#define SUNXI_MMC_TUNING_TIMEOUT_US 10000U

/* JESD84-B51 CMD21 patterns for 4-bit and 8-bit eMMC buses. */
static const uint8_t sunxi_mmc_tuning_pattern_4bit[64] = {
	0xff, 0x0f, 0xff, 0x00, 0xff, 0xcc, 0xc3, 0xcc,
	0xc3, 0x3c, 0xcc, 0xff, 0xfe, 0xff, 0xfe, 0xef,
	0xff, 0xdf, 0xff, 0xdd, 0xff, 0xfb, 0xff, 0xfb,
	0xbf, 0xff, 0x7f, 0xff, 0x77, 0xf7, 0xbd, 0xef,
	0xff, 0xf0, 0xff, 0xf0, 0x0f, 0xfc, 0xcc, 0x3c,
	0xcc, 0x33, 0xcc, 0xcf, 0xff, 0xef, 0xff, 0xee,
	0xff, 0xfd, 0xff, 0xfd, 0xdf, 0xff, 0xbf, 0xff,
	0xbb, 0xff, 0xf7, 0xff, 0xf7, 0x7f, 0x7b, 0xde,
};

static const uint8_t sunxi_mmc_tuning_pattern_8bit[128] = {
	0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0x00,
	0xff, 0xff, 0xcc, 0xcc, 0xcc, 0x33, 0xcc, 0xcc,
	0xcc, 0x33, 0x33, 0xcc, 0xcc, 0xcc, 0xff, 0xff,
	0xff, 0xee, 0xff, 0xff, 0xff, 0xee, 0xee, 0xff,
	0xff, 0xff, 0xdd, 0xff, 0xff, 0xff, 0xdd, 0xdd,
	0xff, 0xff, 0xff, 0xbb, 0xff, 0xff, 0xff, 0xbb,
	0xbb, 0xff, 0xff, 0xff, 0x77, 0xff, 0xff, 0xff,
	0x77, 0x77, 0xff, 0x77, 0xbb, 0xdd, 0xee, 0xff,
	0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00,
	0x00, 0xff, 0xff, 0xcc, 0xcc, 0xcc, 0x33, 0xcc,
	0xcc, 0xcc, 0x33, 0x33, 0xcc, 0xcc, 0xcc, 0xff,
	0xff, 0xff, 0xee, 0xff, 0xff, 0xff, 0xee, 0xee,
	0xff, 0xff, 0xff, 0xdd, 0xff, 0xff, 0xff, 0xdd,
	0xdd, 0xff, 0xff, 0xff, 0xbb, 0xff, 0xff, 0xff,
	0xbb, 0xbb, 0xff, 0xff, 0xff, 0x77, 0xff, 0xff,
	0xff, 0x77, 0x77, 0xff, 0x77, 0xbb, 0xdd, 0xee,
};

static uint32_t sunxi_mmc_tuning_freq_id(uint32_t clock)
{
	if (clock <= 400000U)
		return MMC_CLK_400K;
	if (clock <= 26000000U)
		return MMC_CLK_25M;
	if (clock <= 52000000U)
		return MMC_CLK_50M;
	if (clock <= 100000000U)
		return MMC_CLK_100M;
	if (clock <= 150000000U)
		return MMC_CLK_150M;
	return MMC_CLK_200M;
}

static void sunxi_mmc_tuning_set_fifo_bypass(sunxi_sdhci_t *sdhci, bool bypass)
{
#if defined(CONFIG_SOC_SUN55IW3) || defined(CONFIG_SOC_SUN60IW2) || \
	defined(CONFIG_SOC_SUN55IW6) || defined(CONFIG_SOC_SUN65IW1) || \
	defined(CONFIG_SOC_SUN8IW22)
	uint32_t value;

	if (sdhci == NULL || sdhci->id != MMC_CONTROLLER_2 ||
	    sdhci->mmc_host.timing_mode != SUNXI_MMC_TIMING_MODE_4 ||
	    sdhci->mmc_host.reg == NULL)
		return;

	value = sdhci->mmc_host.reg->sfc;
	if (bypass)
		value |= SMHC_SFC_SAMPLE_FIFO_BYPASS;
	else
		value &= ~SMHC_SFC_SAMPLE_FIFO_BYPASS;
	sdhci->mmc_host.reg->sfc = value;
#else
	(void)sdhci;
	(void)bypass;
#endif
}

static void sunxi_mmc_tuning_set_sample(sunxi_sdhci_t *sdhci, uint32_t delay)
{
	sdhci_reg_t *reg = sdhci->mmc_host.reg;
	uint32_t value = reg->samp_dl;

	value &= ~SDXC_NTDC_CFG_DLY;
	value |= (delay & SDXC_NTDC_CFG_DLY) | SDXC_NTDC_ENABLE_DLY;
	reg->samp_dl = value;
}

static uint32_t sunxi_mmc_tuning_select(const uint8_t *pass);

#if CONFIG_DRIVER_MMC_TUNING
static void sunxi_mmc_tuning_set_data_strobe(sunxi_sdhci_t *sdhci, uint32_t delay)
{
	sdhci_reg_t *reg = sdhci->mmc_host.reg;
	uint32_t value = reg->ds_dl;

	value &= ~SDXC_NTDC_CFG_DLY;
	value |= (delay & SDXC_NTDC_CFG_DLY) | SDXC_NTDC_ENABLE_DLY;
	reg->ds_dl = value;
}
#endif

static int sunxi_mmc_send_tuning(sunxi_sdhci_t *sdhci, const uint8_t *pattern, uint32_t size)
{
	mmc_cmd_t cmd = { 0 };
	mmc_data_t data = { 0 };
	uint8_t response[128] __attribute__((aligned(4)));

	cmd.cmdidx = MMC_CMD_SEND_TUNING_BLOCK;
	cmd.resp_type = MMC_RSP_R1;

	data.b.dest = (char *)response;
	data.flags = MMC_DATA_READ;
	data.blocks = 1;
	data.blocksize = size;

	if (sunxi_sdhci_xfer_timeout(sdhci, &cmd, &data,
				      SUNXI_MMC_TUNING_TIMEOUT_US)) {
		/* A bad delay can reset the controller. Reapply the active timing
		 * configuration before trying the next sample point. */
		sunxi_sdhci_set_ios(sdhci);
		return -1;
	}

	return memcmp(response, pattern, size) == 0 ? 0 : -1;
}

#if CONFIG_DRIVER_MMC_TUNING
static int sunxi_mmc_send_hs400_command_test(sunxi_sdhci_t *sdhci)
{
	mmc_cmd_t cmd = { 0 };

	cmd.cmdidx = MMC_CMD_SEND_STATUS;
	cmd.resp_type = MMC_RSP_R1;
	cmd.cmdarg = sdhci->mmc.rca << 16;

	if (sunxi_sdhci_xfer_timeout(sdhci, &cmd, NULL,
				      SUNXI_MMC_TUNING_TIMEOUT_US)) {
		/* Reapply the last committed delay after a bad command point. */
		sunxi_sdhci_set_ios(sdhci);
		return -1;
	}
	if (!(cmd.response[0] & MMC_STATUS_RDY_FOR_DATA) || (cmd.response[0] & MMC_STATUS_MASK))
		return -1;

	return 0;
}

int sunxi_mmc_execute_hs400_command_tuning(sunxi_sdhci_t *sdhci)
{
	mmc_t *mmc;
	uint8_t pass[SUNXI_MMC_TUNING_POINTS] = { 0 };
	uint32_t freq_id;
	uint32_t selected;

	if (sdhci == NULL)
		return -1;

	mmc = &sdhci->mmc;
	if (sdhci->id != MMC_CONTROLLER_2 || sdhci->mmc_host.timing_mode != SUNXI_MMC_TIMING_MODE_4 ||
	    mmc->speed_mode != MMC_HS400 || mmc->bus_width != SMHC_WIDTH_8BIT)
		return -1;

	freq_id = sunxi_mmc_tuning_freq_id(mmc->clock);
	for (uint32_t delay = 0; delay < SUNXI_MMC_TUNING_POINTS; ++delay) {
		/* Error recovery resets SFC, so restore the tuning mode per point. */
		sunxi_mmc_tuning_set_fifo_bypass(sdhci, true);
		sunxi_mmc_tuning_set_sample(sdhci, delay);
		pass[delay] = sunxi_mmc_send_hs400_command_test(sdhci) == 0;
	}

	selected = sunxi_mmc_tuning_select(pass);
	if (selected == SUNXI_MMC_TUNING_INVALID) {
		sunxi_mmc_tuning_set_fifo_bypass(sdhci, false);
		pr_warn("no valid HS400 command window at %uHz\n", mmc->clock);
		return -1;
	}

	sunxi_mmc_tuning_set_sample(sdhci, selected);
	sunxi_mmc_tuning_set_fifo_bypass(sdhci, false);
	{
		uint32_t index = MMC_HS400 * 2U + freq_id / 4U;
		uint32_t shift = (freq_id % 4U) * 8U;
		uint32_t value = mmc->tune_sdly.tm4_smx_fx[index];

		value &= ~(0xffU << shift);
		value |= selected << shift;
		mmc->tune_sdly.tm4_smx_fx[index] = value;
	}

	pr_info("HS400 command delay %u selected at %uHz\n", selected, mmc->clock);
	return 0;
}

int sunxi_mmc_capture_hs400_reference(sunxi_sdhci_t *sdhci, uint8_t *reference)
{
	mmc_cmd_t cmd = { 0 };
	mmc_data_t data = { 0 };

	if (sdhci == NULL || reference == NULL)
		return -1;

	/* Capture known card data while the HS200 sample point is valid. */
	cmd.cmdidx = MMC_CMD_READ_SINGLE_BLOCK;
	cmd.resp_type = MMC_RSP_R1;
	cmd.cmdarg = 0;
	data.b.dest = (char *)reference;
	data.flags = MMC_DATA_READ;
	data.blocks = 1;
	data.blocksize = SUNXI_MMC_TUNING_BLOCK_SIZE;

	if (sunxi_sdhci_xfer_timeout(sdhci, &cmd, &data,
				      SUNXI_MMC_TUNING_TIMEOUT_US)) {
		sunxi_sdhci_set_ios(sdhci);
		return -1;
	}

	return 0;
}

static int sunxi_mmc_read_hs400_tuning_block(sunxi_sdhci_t *sdhci, uint8_t *buffer)
{
	mmc_cmd_t cmd = { 0 };
	mmc_data_t data = { 0 };

	cmd.cmdidx = MMC_CMD_READ_SINGLE_BLOCK;
	cmd.resp_type = MMC_RSP_R1;
	cmd.cmdarg = 0;

	data.b.dest = (char *)buffer;
	data.flags = MMC_DATA_READ;
	data.blocks = 1;
	data.blocksize = SUNXI_MMC_TUNING_BLOCK_SIZE;

	if (sunxi_sdhci_xfer_timeout(sdhci, &cmd, &data,
				      SUNXI_MMC_TUNING_TIMEOUT_US)) {
		sunxi_sdhci_set_ios(sdhci);
		return -1;
	}

	return 0;
}
#endif

static uint32_t sunxi_mmc_tuning_select(const uint8_t *pass)
{
	uint32_t best_start = 0;
	uint32_t best_length = 0;

	for (uint32_t start = 0; start < SUNXI_MMC_TUNING_POINTS; ++start) {
		uint32_t length = 0;

		if (!pass[start])
			continue;

		while (start + length < SUNXI_MMC_TUNING_POINTS && pass[start + length])
			++length;

		if (length > best_length) {
			best_start = start;
			best_length = length;
		}
	}

	if (best_length < SUNXI_MMC_TUNING_MIN_WINDOW)
		return SUNXI_MMC_TUNING_INVALID;

	return best_start + best_length / 2U;
}

int sunxi_mmc_execute_tuning(sunxi_sdhci_t *sdhci)
{
	mmc_t *mmc;
	const uint8_t *pattern;
	uint32_t pattern_size;
	uint32_t freq_id;
	uint8_t pass[SUNXI_MMC_TUNING_POINTS] = { 0 };
	uint32_t selected;

	if (sdhci == NULL)
		return -1;

	mmc = &sdhci->mmc;
	if (sdhci->id != MMC_CONTROLLER_2 || sdhci->mmc_host.timing_mode != SUNXI_MMC_TIMING_MODE_4 ||
	    mmc->speed_mode != MMC_HS200_SDR104 ||
	    (mmc->bus_width != SMHC_WIDTH_4BIT && mmc->bus_width != SMHC_WIDTH_8BIT))
		return -1;

	if (mmc->bus_width == SMHC_WIDTH_8BIT) {
		pattern = sunxi_mmc_tuning_pattern_8bit;
		pattern_size = sizeof(sunxi_mmc_tuning_pattern_8bit);
	} else {
		pattern = sunxi_mmc_tuning_pattern_4bit;
		pattern_size = sizeof(sunxi_mmc_tuning_pattern_4bit);
	}

	freq_id = sunxi_mmc_tuning_freq_id(mmc->clock);
	for (uint32_t delay = 0; delay < SUNXI_MMC_TUNING_POINTS; ++delay) {
		/* Error recovery resets SFC, so restore the tuning mode per point. */
		sunxi_mmc_tuning_set_fifo_bypass(sdhci, true);
		sunxi_mmc_tuning_set_sample(sdhci, delay);
		pass[delay] = sunxi_mmc_send_tuning(sdhci, pattern, pattern_size) == 0;
	}

	selected = sunxi_mmc_tuning_select(pass);
	if (selected == SUNXI_MMC_TUNING_INVALID) {
		sunxi_mmc_tuning_set_fifo_bypass(sdhci, false);
		pr_warn("no valid sample window at %uHz\n", mmc->clock);
		return -1;
	}

	sunxi_mmc_tuning_set_sample(sdhci, selected);
	sunxi_mmc_tuning_set_fifo_bypass(sdhci, false);
	{
		uint32_t index = MMC_HS200_SDR104 * 2U + freq_id / 4U;
		uint32_t shift = (freq_id % 4U) * 8U;
		uint32_t value = mmc->tune_sdly.tm4_smx_fx[index];

		value &= ~(0xffU << shift);
		value |= selected << shift;
		mmc->tune_sdly.tm4_smx_fx[index] = value;
	}

	pr_info("HS200 sample delay %u selected at %uHz\n", selected, mmc->clock);
	return 0;
}

#if CONFIG_DRIVER_MMC_TUNING
int sunxi_mmc_execute_hs400_tuning(sunxi_sdhci_t *sdhci, const uint8_t *reference)
{
	mmc_t *mmc;
	uint8_t received[SUNXI_MMC_TUNING_BLOCK_SIZE] __attribute__((aligned(4)));
	uint8_t pass[SUNXI_MMC_TUNING_POINTS] = { 0 };
	uint32_t freq_id;
	uint32_t selected;

	if (sdhci == NULL || reference == NULL)
		return -1;

	mmc = &sdhci->mmc;
	if (sdhci->id != MMC_CONTROLLER_2 || sdhci->mmc_host.timing_mode != SUNXI_MMC_TIMING_MODE_4 ||
	    mmc->speed_mode != MMC_HS400 ||
	    mmc->bus_width != SMHC_WIDTH_8BIT || mmc->capacity < SUNXI_MMC_TUNING_BLOCK_SIZE)
		return -1;

	freq_id = sunxi_mmc_tuning_freq_id(mmc->clock);
	/* The vendor TM4 flow only bypasses the sample FIFO for command tuning. */
	/* Scan all points against data captured from the tuned HS200 link. */
	for (uint32_t delay = 0; delay < SUNXI_MMC_TUNING_POINTS; ++delay) {
		sunxi_mmc_tuning_set_fifo_bypass(sdhci, false);
		sunxi_mmc_tuning_set_data_strobe(sdhci, delay);
		if (!sunxi_mmc_read_hs400_tuning_block(sdhci, received) &&
		    memcmp(received, reference, SUNXI_MMC_TUNING_BLOCK_SIZE) == 0)
			pass[delay] = 1;
	}

	selected = sunxi_mmc_tuning_select(pass);
	if (selected == SUNXI_MMC_TUNING_INVALID) {
		sunxi_mmc_tuning_set_fifo_bypass(sdhci, false);
		pr_warn("no valid data-strobe window at %uHz\n", mmc->clock);
		return -1;
	}

	sunxi_mmc_tuning_set_data_strobe(sdhci, selected);
	sunxi_mmc_tuning_set_fifo_bypass(sdhci, false);
	mmc->tune_sdly.tm4_dsdly[freq_id] = selected;
	pr_info("HS400 data-strobe delay %u selected at %uHz\n", selected, mmc->clock);

	return 0;
}
#endif
