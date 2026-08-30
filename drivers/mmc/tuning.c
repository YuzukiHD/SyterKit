/* SPDX-License-Identifier: GPL-2.0+ */
#define pr_fmt(fmt) "SMHC: " fmt

#include <stdint.h>

#include <log.h>
#include <string.h>

#include <drivers/mmc/mmc.h>
#include <drivers/mmc/sdhci.h>
#include <drivers/mmc/tuning.h>
#include <drivers/mmc/hs-timing.h>

#define SUNXI_MMC_TUNING_POINTS	    64U
#define SUNXI_MMC_TUNING_MIN_WINDOW 12U
#define SUNXI_MMC_TUNING_INVALID    0xffffffffU
#define SUNXI_MMC_TUNING_BLOCK_SIZE 512U
#define SUNXI_MMC_TUNING_LEN	    60U
#define SUNXI_MMC_TUNING_LBA	    (24576U - 4U - SUNXI_MMC_TUNING_LEN)
#define SUNXI_MMC_TUNING_MAX_BLOCKS 10U
#define SUNXI_MMC_TUNING_TIMEOUT_US 10000U
#define SUNXI_MMC_TUNING_SAFE_CLOCK 50000000U

/* Keep the vendor method-0 tuning data in the same reserved area and shape. */
#define SUNXI_MMC_TUNING_PATTERNS_PER_LINE 4U
#define SUNXI_MMC_TUNING_PATTERN_8BIT	   128U
#define SUNXI_MMC_TUNING_PATTERN_4BIT	   64U

static const uint8_t sunxi_mmc_tuning_pattern_4bit[64] = {
	0xff,
	0x0f,
	0xff,
	0x00,
	0xff,
	0xcc,
	0xc3,
	0xcc,
	0xc3,
	0x3c,
	0xcc,
	0xff,
	0xfe,
	0xff,
	0xfe,
	0xef,
	0xff,
	0xdf,
	0xff,
	0xdd,
	0xff,
	0xfb,
	0xff,
	0xfb,
	0xbf,
	0xff,
	0x7f,
	0xff,
	0x77,
	0xf7,
	0xbd,
	0xef,
	0xff,
	0xf0,
	0xff,
	0xf0,
	0x0f,
	0xfc,
	0xcc,
	0x3c,
	0xcc,
	0x33,
	0xcc,
	0xcf,
	0xff,
	0xef,
	0xff,
	0xee,
	0xff,
	0xfd,
	0xff,
	0xfd,
	0xdf,
	0xff,
	0xbf,
	0xff,
	0xbb,
	0xff,
	0xf7,
	0xff,
	0xf7,
	0x7f,
	0x7b,
	0xde,
};

static const uint8_t sunxi_mmc_tuning_pattern_8bit[128] = {
	0xff,
	0xff,
	0x00,
	0xff,
	0xff,
	0xff,
	0x00,
	0x00,
	0xff,
	0xff,
	0xcc,
	0xcc,
	0xcc,
	0x33,
	0xcc,
	0xcc,
	0xcc,
	0x33,
	0x33,
	0xcc,
	0xcc,
	0xcc,
	0xff,
	0xff,
	0xff,
	0xee,
	0xff,
	0xff,
	0xff,
	0xee,
	0xee,
	0xff,
	0xff,
	0xff,
	0xdd,
	0xff,
	0xff,
	0xff,
	0xdd,
	0xdd,
	0xff,
	0xff,
	0xff,
	0xbb,
	0xff,
	0xff,
	0xff,
	0xbb,
	0xbb,
	0xff,
	0xff,
	0xff,
	0x77,
	0xff,
	0xff,
	0xff,
	0x77,
	0x77,
	0xff,
	0x77,
	0xbb,
	0xdd,
	0xee,
	0xff,
	0xff,
	0xff,
	0xff,
	0x00,
	0xff,
	0xff,
	0xff,
	0x00,
	0x00,
	0xff,
	0xff,
	0xcc,
	0xcc,
	0xcc,
	0x33,
	0xcc,
	0xcc,
	0xcc,
	0x33,
	0x33,
	0xcc,
	0xcc,
	0xcc,
	0xff,
	0xff,
	0xff,
	0xee,
	0xff,
	0xff,
	0xff,
	0xee,
	0xee,
	0xff,
	0xff,
	0xff,
	0xdd,
	0xff,
	0xff,
	0xff,
	0xdd,
	0xdd,
	0xff,
	0xff,
	0xff,
	0xbb,
	0xff,
	0xff,
	0xff,
	0xbb,
	0xbb,
	0xff,
	0xff,
	0xff,
	0x77,
	0xff,
	0xff,
	0xff,
	0x77,
	0x77,
	0xff,
	0x77,
	0xbb,
	0xdd,
	0xee,
};

static const uint8_t sunxi_mmc_tuning_extra_wifi[64] = {
	0x30,
	0x43,
	0x04,
	0x16,
	0x00,
	0x90,
	0x10,
	0x18,
	0x01,
	0x00,
	0xf8,
	0x4b,
	0x11,
	0x42,
	0x00,
	0x27,
	0x03,
	0x00,
	0x00,
	0x00,
	0x05,
	0x00,
	0x00,
	0x18,
	0xc5,
	0x00,
	0x10,
	0x18,
	0x01,
	0x12,
	0xf8,
	0x4b,
	0x11,
	0x42,
	0x00,
	0x19,
	0x03,
	0x01,
	0x00,
	0x00,
	0x05,
	0x10,
	0x00,
	0x18,
	0xc5,
	0x10,
	0x10,
	0x18,
	0x30,
	0x43,
	0x04,
	0x16,
	0x00,
	0x90,
	0x10,
	0x18,
	0x01,
	0x00,
	0xf8,
	0x4b,
	0x11,
	0x42,
	0x00,
	0x27,
};

static const uint8_t sunxi_mmc_tuning_extra_random[128] = {
	0xe4,
	0x4f,
	0x76,
	0xbb,
	0xf0,
	0xb7,
	0xe0,
	0xdb,
	0xb9,
	0x1f,
	0x9f,
	0xfb,
	0x7e,
	0x9b,
	0x03,
	0x7d,
	0x2e,
	0x32,
	0x8f,
	0x29,
	0x7a,
	0x9b,
	0xab,
	0x16,
	0x2f,
	0x44,
	0x99,
	0xce,
	0xc3,
	0x99,
	0xaa,
	0xad,
	0x2d,
	0x82,
	0xb2,
	0x8a,
	0xfa,
	0x2d,
	0xb9,
	0x9a,
	0x9e,
	0x0f,
	0xf3,
	0x90,
	0x08,
	0x25,
	0xf3,
	0x09,
	0x79,
	0x80,
	0x1b,
	0x28,
	0x95,
	0x00,
	0x57,
	0x7d,
	0xbb,
	0x60,
	0x0b,
	0x2c,
	0x92,
	0x72,
	0x49,
	0x4b,
	0xe4,
	0xac,
	0x48,
	0x8b,
	0xb0,
	0xe4,
	0x11,
	0x1b,
	0x7a,
	0x58,
	0x7c,
	0xc9,
	0xe6,
	0xf1,
	0x5b,
	0x6b,
	0x85,
	0xc9,
	0xf5,
	0x7d,
	0xef,
	0xea,
	0xb6,
	0x0b,
	0x12,
	0x59,
	0x24,
	0xd2,
	0xc9,
	0x53,
	0x15,
	0xa2,
	0xb1,
	0xd6,
	0x1f,
	0x06,
	0x38,
	0x63,
	0x51,
	0x27,
	0xf6,
	0x03,
	0x20,
	0xee,
	0x41,
	0x88,
	0xa4,
	0x69,
	0xfb,
	0x15,
	0x05,
	0x70,
	0xaf,
	0xe0,
	0x30,
	0x88,
	0xdc,
	0x37,
	0xce,
	0x07,
	0x91,
	0xc1,
	0x76,
	0x79,
};

static const uint8_t sunxi_mmc_tuning_seed[4][2] = {
	{ 0xfe, 0x01 },
	{ 0x01, 0xfe },
	{ 0x00, 0xfe },
	{ 0x01, 0xff },
};

static uint8_t sunxi_mmc_tuning_block_4bit[6U * SUNXI_MMC_TUNING_BLOCK_SIZE] __attribute__((aligned(64)));
static uint8_t sunxi_mmc_tuning_block_8bit[10U * SUNXI_MMC_TUNING_BLOCK_SIZE] __attribute__((aligned(64)));
static uint8_t sunxi_mmc_tuning_readback[SUNXI_MMC_TUNING_MAX_BLOCKS * SUNXI_MMC_TUNING_BLOCK_SIZE]
	__attribute__((aligned(64)));
static uint32_t sunxi_mmc_tuning_blocks_4bit;
static uint32_t sunxi_mmc_tuning_blocks_8bit;
static bool sunxi_mmc_tuning_pattern_ready;
static sunxi_sdhci_t *sunxi_mmc_tuning_pattern_host;
static uint32_t sunxi_mmc_tuning_pattern_width;

void sunxi_mmc_tuning_reset(void)
{
	sunxi_mmc_tuning_pattern_host = NULL;
	sunxi_mmc_tuning_pattern_width = 0U;
}

static void sunxi_mmc_tuning_fill_line4(uint8_t *data, uint32_t bit, uint32_t size)
{
	uint32_t pattern;
	uint32_t i;
	uint32_t repeat = size >> 1;
	uint32_t offset = 0;

	for (pattern = 0; pattern < SUNXI_MMC_TUNING_PATTERNS_PER_LINE; pattern++) {
		uint8_t first = sunxi_mmc_tuning_seed[pattern][0];
		uint8_t second = sunxi_mmc_tuning_seed[pattern][1];

		for (i = 1; i < bit; i++)
			first = (uint8_t)(((first << 1) & 0xfU) | ((first >> 3) & 1U));
		for (i = 1; i <= bit; i++)
			second = (uint8_t)(((second << 1) & 0xfU) | ((second >> 3) & 1U));

		first = (uint8_t)((first << 4) | first);
		second = (uint8_t)((second << 4) | second);
		for (i = 0; i < repeat; i++) {
			data[offset++] = first;
			data[offset++] = second;
		}
	}
}

static void sunxi_mmc_tuning_fill_line8(uint8_t *data, uint32_t bit, uint32_t size)
{
	uint32_t pattern;
	uint32_t i;
	uint32_t repeat = size >> 1;
	uint32_t offset = 0;

	for (pattern = 0; pattern < SUNXI_MMC_TUNING_PATTERNS_PER_LINE; pattern++) {
		uint8_t first = sunxi_mmc_tuning_seed[pattern][0];
		uint8_t second = sunxi_mmc_tuning_seed[pattern][1];

		for (i = 1; i <= bit; i++)
			first = (uint8_t)((first << 1) | (first >> 7));
		for (i = 1; i <= bit; i++)
			second = (uint8_t)((second << 1) | (second >> 7));

		for (i = 0; i < repeat; i++) {
			data[offset++] = first;
			data[offset++] = second;
		}
	}
}

static uint32_t sunxi_mmc_tuning_fill_block(uint8_t *data, bool bus8)
{
	uint32_t block_count = 0;
	uint32_t i;

	if (bus8) {
		for (i = 0; i < SUNXI_MMC_TUNING_BLOCK_SIZE; i++)
			data[i] = sunxi_mmc_tuning_pattern_8bit[i % sizeof(sunxi_mmc_tuning_pattern_8bit)];
		block_count = 1;
		for (i = 0; i < 8; i++)
			sunxi_mmc_tuning_fill_line8(
				data + block_count * SUNXI_MMC_TUNING_BLOCK_SIZE +
					i * SUNXI_MMC_TUNING_PATTERNS_PER_LINE * SUNXI_MMC_TUNING_PATTERN_8BIT,
				i, SUNXI_MMC_TUNING_PATTERN_8BIT);
		block_count += 8;
	} else {
		for (i = 0; i < SUNXI_MMC_TUNING_BLOCK_SIZE; i++)
			data[i] = sunxi_mmc_tuning_pattern_4bit[i % sizeof(sunxi_mmc_tuning_pattern_4bit)];
		block_count = 1;
		for (i = 0; i < 8; i++)
			sunxi_mmc_tuning_fill_line4(
				data + block_count * SUNXI_MMC_TUNING_BLOCK_SIZE +
					i * SUNXI_MMC_TUNING_PATTERNS_PER_LINE * SUNXI_MMC_TUNING_PATTERN_4BIT,
				i, SUNXI_MMC_TUNING_PATTERN_8BIT);
		block_count += 4;
	}

	/* Fill the final block with random, WiFi, 00/ff and ff/00 data. */
	for (i = 0; i < 128; i++)
		data[block_count * SUNXI_MMC_TUNING_BLOCK_SIZE + i] = sunxi_mmc_tuning_extra_random[i];
	for (i = 128; i < 256; i++)
		data[block_count * SUNXI_MMC_TUNING_BLOCK_SIZE + i] = sunxi_mmc_tuning_extra_wifi[i % 64U];
	for (i = 128; i < 192; i++) {
		data[block_count * SUNXI_MMC_TUNING_BLOCK_SIZE + 2U * i] = 0x00;
		data[block_count * SUNXI_MMC_TUNING_BLOCK_SIZE + 2U * i + 1U] = 0xff;
	}
	for (i = 192; i < 256; i++) {
		data[block_count * SUNXI_MMC_TUNING_BLOCK_SIZE + 2U * i] = 0xff;
		data[block_count * SUNXI_MMC_TUNING_BLOCK_SIZE + 2U * i + 1U] = 0x00;
	}

	return block_count + 1U;
}

static int sunxi_mmc_tuning_prepare_pattern(void)
{
	if (sunxi_mmc_tuning_pattern_ready)
		return 0;

	sunxi_mmc_tuning_blocks_8bit = sunxi_mmc_tuning_fill_block(sunxi_mmc_tuning_block_8bit, true);
	sunxi_mmc_tuning_blocks_4bit = sunxi_mmc_tuning_fill_block(sunxi_mmc_tuning_block_4bit, false);
	sunxi_mmc_tuning_pattern_ready = true;
	return 0;
}

static int sunxi_mmc_tuning_send_manual_stop(sunxi_sdhci_t *sdhci)
{
	mmc_cmd_t cmd = { 0 };

	cmd.cmdidx = MMC_CMD_STOP_TRANSMISSION;
	cmd.resp_type = MMC_RSP_R1b;
	cmd.flags = MMC_CMD_MANUAL;

	if (sunxi_sdhci_xfer_timeout(sdhci, &cmd, NULL, SUNXI_MMC_TUNING_TIMEOUT_US))
		return -1;

	return 0;
}

static void sunxi_mmc_tuning_restore_link(sunxi_sdhci_t *sdhci)
{
	/* xfer() resets the host after an error; restore mode, width and delays. */
	sunxi_sdhci_set_ios(sdhci);
}

static int sunxi_mmc_tuning_select_pattern(sunxi_sdhci_t *sdhci, const uint8_t **pattern, uint32_t *blocks)
{
	if (sdhci->mmc.bus_width == SMHC_WIDTH_4BIT) {
		*pattern = sunxi_mmc_tuning_block_4bit;
		*blocks = sunxi_mmc_tuning_blocks_4bit;
		return 0;
	}
	if (sdhci->mmc.bus_width == SMHC_WIDTH_8BIT) {
		*pattern = sunxi_mmc_tuning_block_8bit;
		*blocks = sunxi_mmc_tuning_blocks_8bit;
		return 0;
	}

	pr_warn("tuning does not support bus width %u\n", sdhci->mmc.bus_width);
	return -1;
}

static int sunxi_mmc_tuning_card_pattern(sunxi_sdhci_t *sdhci)
{
	mmc_t *mmc = &sdhci->mmc;
	const uint8_t *pattern;
	uint32_t blocks;
	uint32_t original_clock = mmc->clock;
	uint32_t safe_clock = original_clock;
	uint32_t ret;
	int err = -1;
	bool transfer_failed = false;

	if (sunxi_mmc_tuning_prepare_pattern() || sunxi_mmc_tuning_select_pattern(sdhci, &pattern, &blocks))
		return -1;

	if (sunxi_mmc_tuning_pattern_host == sdhci && sunxi_mmc_tuning_pattern_width == mmc->bus_width)
		return 0;

	if (SUNXI_MMC_TUNING_LBA > mmc->lba || blocks > mmc->lba - SUNXI_MMC_TUNING_LBA)
		return -1;

	if (safe_clock > SUNXI_MMC_TUNING_SAFE_CLOCK)
		safe_clock = SUNXI_MMC_TUNING_SAFE_CLOCK;
	if (safe_clock < mmc->f_min)
		safe_clock = mmc->f_min;

	sunxi_mmc_hs_set_clock(sdhci, safe_clock);
	if (sdhci->mmc_host.fatal_err)
		goto out;

	ret = sunxi_mmc_blk_write(sdhci, (void *)pattern, SUNXI_MMC_TUNING_LBA, blocks);
	if (ret != blocks) {
		transfer_failed = true;
		goto out;
	}

	ret = sunxi_mmc_blk_read(sdhci, sunxi_mmc_tuning_readback, SUNXI_MMC_TUNING_LBA, blocks);
	if (ret != blocks) {
		transfer_failed = true;
		goto out;
	}

	if (memcmp(pattern, sunxi_mmc_tuning_readback, blocks * SUNXI_MMC_TUNING_BLOCK_SIZE) != 0)
		goto out;

	err = 0;

out:
	if (transfer_failed) {
		sunxi_mmc_tuning_send_manual_stop(sdhci);
		sunxi_mmc_tuning_restore_link(sdhci);
	}
	sunxi_mmc_hs_set_clock(sdhci, original_clock);
	if (sdhci->mmc_host.fatal_err)
		err = -1;

	if (!err) {
		sunxi_mmc_tuning_pattern_host = sdhci;
		sunxi_mmc_tuning_pattern_width = mmc->bus_width;
	}

	return err;
}

static int sunxi_mmc_tuning_read_pattern(sunxi_sdhci_t *sdhci, const uint8_t *pattern, uint32_t blocks)
{
	uint32_t ret;

	ret = sunxi_mmc_blk_read(sdhci, sunxi_mmc_tuning_readback, SUNXI_MMC_TUNING_LBA, blocks);
	if (ret != blocks) {
		sunxi_mmc_tuning_send_manual_stop(sdhci);
		sunxi_mmc_tuning_restore_link(sdhci);
		return -1;
	}

	return memcmp(pattern, sunxi_mmc_tuning_readback, blocks * SUNXI_MMC_TUNING_BLOCK_SIZE) == 0 ? 0 : -1;
}

static int sunxi_mmc_tuning_method_0(sunxi_sdhci_t *sdhci, const uint8_t *pattern, uint32_t blocks)
{
	return sunxi_mmc_tuning_read_pattern(sdhci, pattern, blocks);
}

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
	uint32_t value;

	if (sdhci == NULL || !sdhci->sample_fifo_bypass || sdhci->id != MMC_CONTROLLER_2 ||
		sdhci->mmc_host.timing_mode != SUNXI_MMC_TIMING_MODE_4 || sdhci->mmc_host.reg == NULL)
		return;

	value = sdhci->mmc_host.reg->sfc;
	if (bypass)
		value |= SMHC_SFC_SAMPLE_FIFO_BYPASS;
	else
		value &= ~SMHC_SFC_SAMPLE_FIFO_BYPASS;
	sdhci->mmc_host.reg->sfc = value;
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
#ifdef CONFIG_DRIVER_MMC_SHOW_TRAINING
static void sunxi_mmc_tuning_dump_chart(const char *name, const uint8_t *pass, uint32_t selected);
#endif

typedef enum {
	SUNXI_MMC_TUNING_HS200,
	SUNXI_MMC_TUNING_HS400_COMMAND,
	SUNXI_MMC_TUNING_HS400_DATA,
} sunxi_mmc_tuning_mode_t;

static void sunxi_mmc_tuning_print_result(sunxi_mmc_tuning_mode_t mode, sunxi_sdhci_t *sdhci, const uint8_t *pass,
	uint32_t selected, uint32_t pattern_blocks);

static void sunxi_mmc_tuning_set_data_strobe(sunxi_sdhci_t *sdhci, uint32_t delay)
{
	sdhci_reg_t *reg = sdhci->mmc_host.reg;
	uint32_t value = reg->ds_dl;

	value &= ~SDXC_NTDC_CFG_DLY;
	value |= (delay & SDXC_NTDC_CFG_DLY) | SDXC_NTDC_ENABLE_DLY;
	reg->ds_dl = value;
}

/* Set the training state and return the previous value for nested tuning. */
static bool sunxi_mmc_training_set(mmc_t *mmc, bool training)
{
	bool previous = mmc->training;

	mmc->training = training;
	return previous;
}

static int sunxi_mmc_send_hs400_command_test(sunxi_sdhci_t *sdhci)
{
	mmc_cmd_t cmd = { 0 };

	cmd.cmdidx = MMC_CMD_SEND_STATUS;
	cmd.resp_type = MMC_RSP_R1;
	cmd.cmdarg = sdhci->mmc.rca << 16;

	if (sunxi_sdhci_xfer_timeout(sdhci, &cmd, NULL, SUNXI_MMC_TUNING_TIMEOUT_US)) {
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
	bool was_training;
	int ret = -1;

	if (sdhci == NULL)
		return -1;

	mmc = &sdhci->mmc;
	if (sdhci->id != MMC_CONTROLLER_2 || sdhci->mmc_host.timing_mode != SUNXI_MMC_TIMING_MODE_4 ||
		mmc->speed_mode != MMC_HS400 || mmc->bus_width != SMHC_WIDTH_8BIT)
		return -1;

	was_training = sunxi_mmc_training_set(mmc, true);
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
		sunxi_mmc_tuning_print_result(SUNXI_MMC_TUNING_HS400_COMMAND, sdhci, pass, selected, 0U);
		goto out;
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

	sunxi_mmc_tuning_print_result(SUNXI_MMC_TUNING_HS400_COMMAND, sdhci, pass, selected, 0U);
	ret = 0;

out:
	sunxi_mmc_training_set(mmc, was_training);
	return ret;
}

#ifdef CONFIG_DRIVER_MMC_SHOW_TRAINING
static void sunxi_mmc_tuning_dump_chart(const char *name, const uint8_t *pass, uint32_t selected)
{
	char samples[SUNXI_MMC_TUNING_POINTS + 1U];
	char selected_line[SUNXI_MMC_TUNING_POINTS + 1U];

	for (uint32_t point = 0U; point < SUNXI_MMC_TUNING_POINTS; ++point)
		samples[point] = pass[point] ? 'O' : '-';
	samples[SUNXI_MMC_TUNING_POINTS] = '\0';

	pr_debug("%s: training chart (O=pass, -=fail)\n", name);
	pr_debug("delay   0         1         2         3         4         5         6\n");
	pr_debug("        0123456789012345678901234567890123456789012345678901234567890123\n");
	pr_debug("result  |%s|\n", samples);

	if (selected >= SUNXI_MMC_TUNING_POINTS)
		return;

	memset(selected_line, ' ', SUNXI_MMC_TUNING_POINTS);
	selected_line[selected] = '^';
	selected_line[SUNXI_MMC_TUNING_POINTS] = '\0';
	pr_debug("select  |%s| delay=%u\n", selected_line, selected);
}
#endif

static void sunxi_mmc_tuning_print_result(sunxi_mmc_tuning_mode_t mode, sunxi_sdhci_t *sdhci, const uint8_t *pass,
	uint32_t selected, uint32_t pattern_blocks)
{
	mmc_t *mmc = &sdhci->mmc;
	uint32_t freq_id = sunxi_mmc_tuning_freq_id(mmc->clock);
	const char *name;

	switch (mode) {
	case SUNXI_MMC_TUNING_HS200:
		name = "HS200/SDR104";
		break;
	case SUNXI_MMC_TUNING_HS400_COMMAND:
		name = "HS400 command";
		break;
	default:
		name = "HS400 data";
		break;
	}

	if (selected == SUNXI_MMC_TUNING_INVALID)
		pr_info("%s: freq=%u clock=%uHz bus=%ubit points=%u selected=invalid\n", name, freq_id, mmc->clock,
			mmc->bus_width == SMHC_WIDTH_8BIT ? 8U : 4U, SUNXI_MMC_TUNING_POINTS);
	else
		pr_info("%s: freq=%u clock=%uHz bus=%ubit points=%u selected=%u\n", name, freq_id, mmc->clock,
			mmc->bus_width == SMHC_WIDTH_8BIT ? 8U : 4U, SUNXI_MMC_TUNING_POINTS, selected);
#ifdef CONFIG_DRIVER_MMC_SHOW_TRAINING
	sunxi_mmc_tuning_dump_chart(name, pass, selected);
#endif

	if (mode == SUNXI_MMC_TUNING_HS200) {
		pr_info("%s: pattern_lba=%u blocks=%u smx_fx=0x%08x 0x%08x\n", name, SUNXI_MMC_TUNING_LBA,
			pattern_blocks, mmc->tune_sdly.tm4_smx_fx[MMC_HS200_SDR104 * 2U],
			mmc->tune_sdly.tm4_smx_fx[MMC_HS200_SDR104 * 2U + 1U]);
	} else if (mode == SUNXI_MMC_TUNING_HS400_DATA) {
		pr_info("%s: dsdly=0x%02x%02x%02x%02x%02x%02x selected=%u\n", name, mmc->tune_sdly.tm4_dsdly[0],
			mmc->tune_sdly.tm4_dsdly[1], mmc->tune_sdly.tm4_dsdly[2], mmc->tune_sdly.tm4_dsdly[3],
			mmc->tune_sdly.tm4_dsdly[4], mmc->tune_sdly.tm4_dsdly[5],
			selected == SUNXI_MMC_TUNING_INVALID ? 0U : selected);
	} else {
		pr_info("%s: smx_fx=0x%08x 0x%08x\n", name, mmc->tune_sdly.tm4_smx_fx[MMC_HS400 * 2U],
			mmc->tune_sdly.tm4_smx_fx[MMC_HS400 * 2U + 1U]);
	}
}

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

	return best_start + (best_length - 1U) / 2U;
}

int sunxi_mmc_execute_tuning(sunxi_sdhci_t *sdhci)
{
	mmc_t *mmc;
	const uint8_t *pattern;
	uint32_t pattern_blocks;
	uint32_t freq_id;
	uint8_t pass[SUNXI_MMC_TUNING_POINTS] = { 0 };
	uint32_t selected;
	bool was_training;
	int ret = -1;

	if (sdhci == NULL)
		return -1;

	mmc = &sdhci->mmc;
	if (sdhci->id != MMC_CONTROLLER_2 || sdhci->mmc_host.timing_mode != SUNXI_MMC_TIMING_MODE_4 ||
		mmc->speed_mode != MMC_HS200_SDR104 ||
		(mmc->bus_width != SMHC_WIDTH_4BIT && mmc->bus_width != SMHC_WIDTH_8BIT))
		return -1;

	was_training = sunxi_mmc_training_set(mmc, true);
	if (sunxi_mmc_tuning_card_pattern(sdhci))
		goto out;
	if (sunxi_mmc_tuning_select_pattern(sdhci, &pattern, &pattern_blocks))
		goto out;

	freq_id = sunxi_mmc_tuning_freq_id(mmc->clock);
	for (uint32_t delay = 0; delay < SUNXI_MMC_TUNING_POINTS; ++delay) {
		/* A failed block transfer resets the host; restore the selected point. */
		sunxi_mmc_tuning_set_fifo_bypass(sdhci, false);
		sunxi_mmc_tuning_set_sample(sdhci, delay);
		pass[delay] = sunxi_mmc_tuning_method_0(sdhci, pattern, pattern_blocks) == 0;
	}

	selected = sunxi_mmc_tuning_select(pass);
	if (selected == SUNXI_MMC_TUNING_INVALID) {
		sunxi_mmc_tuning_set_fifo_bypass(sdhci, false);
		sunxi_mmc_tuning_print_result(SUNXI_MMC_TUNING_HS200, sdhci, pass, selected, pattern_blocks);
		goto out;
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

	sunxi_mmc_tuning_print_result(SUNXI_MMC_TUNING_HS200, sdhci, pass, selected, pattern_blocks);
	ret = 0;

out:
	sunxi_mmc_training_set(mmc, was_training);
	return ret;
}

int sunxi_mmc_execute_hs400_tuning(sunxi_sdhci_t *sdhci)
{
	mmc_t *mmc;
	const uint8_t *pattern;
	uint32_t pattern_blocks;
	uint8_t pass[SUNXI_MMC_TUNING_POINTS] = { 0 };
	uint32_t freq_id;
	uint32_t selected;
	bool was_training;
	int ret = -1;

	if (sdhci == NULL)
		return -1;

	mmc = &sdhci->mmc;
	if (sdhci->id != MMC_CONTROLLER_2 || sdhci->mmc_host.timing_mode != SUNXI_MMC_TIMING_MODE_4 ||
		mmc->speed_mode != MMC_HS400 || mmc->bus_width != SMHC_WIDTH_8BIT ||
		mmc->capacity < SUNXI_MMC_TUNING_BLOCK_SIZE)
		return -1;
	if (sunxi_mmc_tuning_pattern_host != sdhci || sunxi_mmc_tuning_pattern_width != mmc->bus_width ||
		sunxi_mmc_tuning_select_pattern(sdhci, &pattern, &pattern_blocks))
		return -1;

	was_training = sunxi_mmc_training_set(mmc, true);
	freq_id = sunxi_mmc_tuning_freq_id(mmc->clock);
	/* The vendor TM4 flow only bypasses the sample FIFO for command tuning. */
	/* Scan all points against the pattern written through the tuned HS200 link. */
	for (uint32_t delay = 0; delay < SUNXI_MMC_TUNING_POINTS; ++delay) {
		sunxi_mmc_tuning_set_fifo_bypass(sdhci, false);
		sunxi_mmc_tuning_set_data_strobe(sdhci, delay);
		if (!sunxi_mmc_tuning_method_0(sdhci, pattern, pattern_blocks))
			pass[delay] = 1;
	}

	selected = sunxi_mmc_tuning_select(pass);
	if (selected == SUNXI_MMC_TUNING_INVALID) {
		sunxi_mmc_tuning_set_fifo_bypass(sdhci, false);
		sunxi_mmc_tuning_print_result(SUNXI_MMC_TUNING_HS400_DATA, sdhci, pass, selected, pattern_blocks);
		goto out;
	}

	sunxi_mmc_tuning_set_data_strobe(sdhci, selected);
	sunxi_mmc_tuning_set_fifo_bypass(sdhci, false);
	mmc->tune_sdly.tm4_dsdly[freq_id] = selected;

	sunxi_mmc_tuning_print_result(SUNXI_MMC_TUNING_HS400_DATA, sdhci, pass, selected, pattern_blocks);
	ret = 0;

out:
	sunxi_mmc_training_set(mmc, was_training);
	return ret;
}
