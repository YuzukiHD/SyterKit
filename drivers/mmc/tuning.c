/* SPDX-License-Identifier: GPL-2.0+ */
#define pr_fmt(fmt) "SMHC: " fmt

#include <stdbool.h>
#include <stdint.h>

#include <log.h>
#include <malloc.h>
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
#define SUNXI_MMC_TUNING_ALIGNMENT  64U

/* Keep the vendor method-0 layout in the same reserved card area and shape. */
#define SUNXI_MMC_TUNING_PATTERNS_PER_LINE 4U
#define SUNXI_MMC_TUNING_PATTERN_8BIT	   128U
#define SUNXI_MMC_TUNING_PATTERN_4BIT	   64U

static const uint8_t sunxi_mmc_tuning_seed[4][2] = {
	{ 0xfe, 0x01 },
	{ 0x01, 0xfe },
	{ 0x00, 0xfe },
	{ 0x01, 0xff },
};

struct sunxi_mmc_tuning_pattern {
	uint8_t *data;
	uint8_t *readback;
	void *allocation;
	uint32_t blocks;
};

struct sunxi_mmc_tuning_pattern_cache {
	sunxi_sdhci_t *host;
	uint32_t bus_width;
};

typedef enum {
	SUNXI_MMC_TUNING_HS200,
	SUNXI_MMC_TUNING_HS400_COMMAND,
	SUNXI_MMC_TUNING_HS400_DATA,
} sunxi_mmc_tuning_mode_t;

struct sunxi_mmc_tuning_run {
	sunxi_sdhci_t *sdhci;
	mmc_t *mmc;
	uint8_t pass[SUNXI_MMC_TUNING_POINTS];
	uint32_t freq_id;
	uint32_t selected;
	bool previous_training;
	bool active;
};

typedef void (*sunxi_mmc_tuning_delay_fn)(sunxi_sdhci_t *sdhci, uint32_t delay);
typedef int (*sunxi_mmc_tuning_probe_fn)(sunxi_sdhci_t *sdhci, const void *arg);

struct sunxi_mmc_tuning_scan {
	sunxi_mmc_tuning_delay_fn set_delay;
	sunxi_mmc_tuning_probe_fn probe;
	const void *probe_arg;
	bool fifo_bypass;
};

static struct sunxi_mmc_tuning_pattern_cache sunxi_mmc_tuning_patterns;

/**
 * @brief Map a clock frequency to a tuning frequency ID
 * @details Converts a clock frequency in Hz to the corresponding MMC clock
 *          frequency ID used to index tuning tables. Frequencies above
 *          150 MHz map to MMC_CLK_200M.
 * @param clock Clock frequency in Hz
 * @return The matching MMC clock frequency ID
 */
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

/**
 * @brief Reset the cached tuning pattern state
 * @details Clears the cached host and bus width associated with a prepared
 *          tuning pattern, forcing the next tuning run to prepare the card
 *          again.
 */
void sunxi_mmc_tuning_reset(void)
{
	sunxi_mmc_tuning_patterns.host = NULL;
	sunxi_mmc_tuning_patterns.bus_width = 0U;
}

/**
 * @brief Fill a buffer with a PRBS-16 pattern
 * @details Generates a pseudo-random bit sequence using the PRBS-16
 *          polynomial x^16 + x^14 + x^13 + x^11 + 1 and stores one byte per
 *          generated bit into the destination buffer.
 * @param data Pointer to the buffer to fill
 * @param size Number of bytes to generate
 * @param seed Initial state for the shift register
 */
static void sunxi_mmc_tuning_fill_prbs(uint8_t *data, uint32_t size, uint16_t seed)
{
	uint16_t state = seed;

	/* PRBS-16: x^16 + x^14 + x^13 + x^11 + 1. */
	for (uint32_t offset = 0U; offset < size; ++offset) {
		uint8_t value = 0U;

		for (uint32_t bit = 0U; bit < 8U; ++bit) {
			uint16_t feedback = state & 1U;

			value |= (uint8_t)(feedback << bit);
			state >>= 1;
			if (feedback)
				state ^= 0xb400U;
		}
		data[offset] = value;
	}
}

/**
 * @brief Rotate a value within a limited bit width
 * @details Rotates the lower bits of value left by count positions, wrapping
 *          the shifted-out bits around, and masks the result to width bits.
 * @param value The value to rotate
 * @param width Number of significant bits in the value
 * @param count Number of positions to rotate left
 * @return The rotated and masked value
 */
static uint8_t sunxi_mmc_tuning_rotate(uint8_t value, uint32_t width, uint32_t count)
{
	uint32_t mask = (1U << width) - 1U;

	count %= width;
	value &= (uint8_t)mask;
	return (uint8_t)(((value << count) | (value >> (width - count))) & mask);
}

/**
 * @brief Fill a data line with rotated tuning patterns
 * @details Writes the four seed byte pairs, rotated according to the data
 *          line index and bus width, repeatedly across the line. The 4-bit
 *          bus uses an asymmetric rotation while 8-bit uses a plain rotation.
 * @param data Pointer to the start of the line to fill
 * @param bit Data line index used to rotate the patterns
 * @param size Length of the line in bytes
 * @param bus8 True for 8-bit bus patterns, false for 4-bit
 */
static void sunxi_mmc_tuning_fill_line(uint8_t *data, uint32_t bit, uint32_t size, bool bus8)
{
	uint32_t repeat = size / 2U;
	uint32_t offset = 0U;

	for (uint32_t pattern = 0U; pattern < SUNXI_MMC_TUNING_PATTERNS_PER_LINE; ++pattern) {
		uint8_t first = sunxi_mmc_tuning_seed[pattern][0];
		uint8_t second = sunxi_mmc_tuning_seed[pattern][1];

		if (bus8) {
			first = sunxi_mmc_tuning_rotate(first, 8U, bit);
			second = sunxi_mmc_tuning_rotate(second, 8U, bit);
		} else {
			/* Keep the asymmetric rotation used by the vendor 4-bit pattern. */
			first = sunxi_mmc_tuning_rotate(first, 4U, bit == 0U ? 0U : bit - 1U);
			second = sunxi_mmc_tuning_rotate(second, 4U, bit);
			first = (uint8_t)((first << 4) | first);
			second = (uint8_t)((second << 4) | second);
		}

		for (uint32_t repeat_index = 0U; repeat_index < repeat; ++repeat_index) {
			data[offset++] = first;
			data[offset++] = second;
		}
	}
}

/**
 * @brief Fill the complete tuning pattern block layout
 * @details Composes the tuning block from a PRBS section, eight rotated
 *          per-line patterns, and a final tail block combining PRBS and
 *          alternating 00/ff and ff/00 sections.
 * @param data Pointer to the destination buffer for the block layout
 * @param bus8 True for 8-bit bus patterns, false for 4-bit
 * @return The number of 512-byte blocks written
 */
static uint32_t sunxi_mmc_tuning_fill_block(uint8_t *data, bool bus8)
{
	uint32_t block_count = 1U;

	/* This is driver-authored data in a reserved LBA, not CMD21 card data. */
	sunxi_mmc_tuning_fill_prbs(data, SUNXI_MMC_TUNING_BLOCK_SIZE, bus8 ? 0x1d0fU : 0x5a3cU);

	if (bus8) {
		for (uint32_t line = 0U; line < 8U; ++line)
			sunxi_mmc_tuning_fill_line(data + block_count * SUNXI_MMC_TUNING_BLOCK_SIZE +
				line * SUNXI_MMC_TUNING_PATTERNS_PER_LINE * SUNXI_MMC_TUNING_PATTERN_8BIT,
				line, SUNXI_MMC_TUNING_PATTERN_8BIT, true);
		block_count += 8U;
	} else {
		for (uint32_t line = 0U; line < 8U; ++line)
			sunxi_mmc_tuning_fill_line(data + block_count * SUNXI_MMC_TUNING_BLOCK_SIZE +
				line * SUNXI_MMC_TUNING_PATTERNS_PER_LINE * SUNXI_MMC_TUNING_PATTERN_4BIT,
				line, SUNXI_MMC_TUNING_PATTERN_4BIT, false);
		block_count += 4U;
	}

	/* The final block combines PRBS, 00/ff and ff/00 sections. */
	uint8_t *tail = data + block_count * SUNXI_MMC_TUNING_BLOCK_SIZE;
	sunxi_mmc_tuning_fill_prbs(tail, 256U, bus8 ? 0x4b1dU : 0xc35aU);
	for (uint32_t offset = 0U; offset < 64U; ++offset) {
		tail[256U + 2U * offset] = 0x00;
		tail[256U + 2U * offset + 1U] = 0xff;
		tail[384U + 2U * offset] = 0xff;
		tail[384U + 2U * offset + 1U] = 0x00;
	}

	return block_count + 1U;
}

/**
 * @brief Release a tuning pattern's allocation
 * @details Frees the workspace allocated for a tuning pattern and zeroes the
 *          pattern structure. Does nothing if the pattern is NULL.
 * @param pattern Pointer to the tuning pattern to release
 */
static void sunxi_mmc_tuning_release_pattern(struct sunxi_mmc_tuning_pattern *pattern)
{
	if (pattern == NULL)
		return;

	free(pattern->allocation);
	memset(pattern, 0, sizeof(*pattern));
}

/**
 * @brief Allocate and fill a tuning pattern for the card
 * @details Determines the pattern size from the card bus width, allocates an
 *          aligned workspace for the write and readback buffers, and fills it
 *          with the tuning block layout.
 * @param sdhci Pointer to the SDHCI controller structure
 * @param pattern Pointer to the tuning pattern structure to populate
 * @return 0 on success, -1 on failure
 */
static int sunxi_mmc_tuning_get_pattern(sunxi_sdhci_t *sdhci, struct sunxi_mmc_tuning_pattern *pattern)
{
	uint32_t blocks;
	uint32_t size;
	void *allocation;
	uintptr_t aligned;
	bool bus8;

	if (sdhci == NULL || pattern == NULL)
		return -1;

	if (sdhci->mmc.bus_width == SMHC_WIDTH_4BIT) {
		blocks = 6U;
		bus8 = false;
	} else if (sdhci->mmc.bus_width == SMHC_WIDTH_8BIT) {
		blocks = SUNXI_MMC_TUNING_MAX_BLOCKS;
		bus8 = true;
	} else {
		pr_warn("tuning does not support bus width %u\n", sdhci->mmc.bus_width);
		return -1;
	}

	size = blocks * SUNXI_MMC_TUNING_BLOCK_SIZE;
	allocation = malloc(2U * size + SUNXI_MMC_TUNING_ALIGNMENT - 1U);
	if (allocation == NULL) {
		pr_warn("tuning workspace allocation (%u bytes) failed\n", 2U * size);
		return -1;
	}

	aligned = ((uintptr_t)allocation + SUNXI_MMC_TUNING_ALIGNMENT - 1U) &
		~(uintptr_t)(SUNXI_MMC_TUNING_ALIGNMENT - 1U);
	pattern->data = (uint8_t *)aligned;
	pattern->readback = pattern->data + size;
	pattern->allocation = allocation;
	pattern->blocks = sunxi_mmc_tuning_fill_block(pattern->data, bus8);
	if (pattern->blocks != blocks) {
		sunxi_mmc_tuning_release_pattern(pattern);
		return -1;
	}

	return 0;
}

/**
 * @brief Send a manual stop transmission command
 * @details Issues MMC_CMD_STOP_TRANSMISSION with the manual flag set so the
 *          command is sent immediately, using the tuning timeout.
 * @param sdhci Pointer to the SDHCI controller structure
 * @return 0 on success, -1 on failure
 */
static int sunxi_mmc_tuning_send_manual_stop(sunxi_sdhci_t *sdhci)
{
	mmc_cmd_t cmd = { 0 };

	cmd.cmdidx = MMC_CMD_STOP_TRANSMISSION;
	cmd.resp_type = MMC_RSP_R1b;
	cmd.flags = MMC_CMD_MANUAL;
	return sunxi_sdhci_xfer_timeout(sdhci, &cmd, NULL, SUNXI_MMC_TUNING_TIMEOUT_US);
}

/**
 * @brief Restore the host link state after a failed transfer
 * @details Re-applies the host I/O settings because a failed transfer resets
 *          controller state, including the sample FIFO bypass flag.
 * @param sdhci Pointer to the SDHCI controller structure
 */
static void sunxi_mmc_tuning_restore_link(sunxi_sdhci_t *sdhci)
{
	/* A failed transfer resets controller state, including SFC. */
	if (sdhci != NULL)
		sunxi_sdhci_set_ios(sdhci);
}

/**
 * @brief Recover from a failed tuning data transfer
 * @details Sends a manual stop transmission command to abort any stuck data
 *          transfer and then restores the host link state.
 * @param sdhci Pointer to the SDHCI controller structure
 */
static void sunxi_mmc_tuning_recover_data_transfer(sunxi_sdhci_t *sdhci)
{
	sunxi_mmc_tuning_send_manual_stop(sdhci);
	sunxi_mmc_tuning_restore_link(sdhci);
}

/**
 * @brief Set the sample FIFO bypass flag
 * @details Enables or disables the sample FIFO bypass bit in the SFC register
 *          on the supported SMHC2 timing-mode-4 controllers.
 * @param sdhci Pointer to the SDHCI controller structure
 * @param bypass True to enable FIFO bypass, false to disable it
 */
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

/**
 * @brief Set the sampling delay
 * @details Programs the delay value into the sampling delay register while
 *          keeping the delay-chain enable bit set.
 * @param sdhci Pointer to the SDHCI controller structure
 * @param delay Delay value to program
 */
static void sunxi_mmc_tuning_set_sample(sunxi_sdhci_t *sdhci, uint32_t delay)
{
	sdhci_reg_t *reg;
	uint32_t value;

	if (sdhci == NULL || sdhci->mmc_host.reg == NULL)
		return;
	reg = sdhci->mmc_host.reg;
	value = reg->samp_dl & ~SDXC_NTDC_CFG_DLY;
	reg->samp_dl = value | (delay & SDXC_NTDC_CFG_DLY) | SDXC_NTDC_ENABLE_DLY;
}

/**
 * @brief Set the HS400 data-strobe delay
 * @details Programs the delay value into the data-strobe delay register while
 *          keeping the delay-chain enable bit set.
 * @param sdhci Pointer to the SDHCI controller structure
 * @param delay Delay value to program
 */
static void sunxi_mmc_tuning_set_data_strobe(sunxi_sdhci_t *sdhci, uint32_t delay)
{
	sdhci_reg_t *reg;
	uint32_t value;

	if (sdhci == NULL || sdhci->mmc_host.reg == NULL)
		return;
	reg = sdhci->mmc_host.reg;
	value = reg->ds_dl & ~SDXC_NTDC_CFG_DLY;
	reg->ds_dl = value | (delay & SDXC_NTDC_CFG_DLY) | SDXC_NTDC_ENABLE_DLY;
}

/**
 * @brief Set the training flag on the MMC structure
 * @details Updates the mmc->training flag used to suppress non-training log
 *          messages and returns the previous value.
 * @param mmc Pointer to the MMC structure
 * @param training New value for the training flag
 * @return The previous training flag value
 */
static bool sunxi_mmc_tuning_set_training(mmc_t *mmc, bool training)
{
	bool previous;

	if (mmc == NULL)
		return false;
	previous = mmc->training;
	mmc->training = training;
	return previous;
}

/**
 * @brief Begin a tuning run
 * @details Initializes the tuning run structure, records the host and MMC
 *          pointers, enables the training flag, and marks the run as active.
 * @param run Pointer to the tuning run structure to initialize
 * @param sdhci Pointer to the SDHCI controller structure
 * @return True on success, false if either pointer is NULL
 */
static bool sunxi_mmc_tuning_begin(struct sunxi_mmc_tuning_run *run, sunxi_sdhci_t *sdhci)
{
	if (run == NULL || sdhci == NULL)
		return false;

	memset(run, 0, sizeof(*run));
	run->sdhci = sdhci;
	run->mmc = &sdhci->mmc;
	run->selected = SUNXI_MMC_TUNING_INVALID;
	run->previous_training = sunxi_mmc_tuning_set_training(run->mmc, true);
	run->active = true;
	return true;
}

/**
 * @brief End a tuning run
 * @details Disables the sample FIFO bypass, restores the previous training
 *          flag, and marks the run as inactive.
 * @param run Pointer to the tuning run structure
 */
static void sunxi_mmc_tuning_end(struct sunxi_mmc_tuning_run *run)
{
	if (run == NULL || !run->active)
		return;

	sunxi_mmc_tuning_set_fifo_bypass(run->sdhci, false);
	(void)sunxi_mmc_tuning_set_training(run->mmc, run->previous_training);
	run->active = false;
}

/**
 * @brief Scan all delay points for the tuning run
 * @details For each delay point, applies the FIFO bypass setting, programs
 *          the delay, and records whether the probe succeeded in the run's
 *          pass array.
 * @param run Pointer to the tuning run structure
 * @param scan Pointer to the scan configuration with delay and probe callbacks
 */
static void sunxi_mmc_tuning_scan(struct sunxi_mmc_tuning_run *run, const struct sunxi_mmc_tuning_scan *scan)
{
	memset(run->pass, 0, sizeof(run->pass));
	for (uint32_t delay = 0U; delay < SUNXI_MMC_TUNING_POINTS; ++delay) {
		/* Error recovery may reset SFC, so apply both settings for every point. */
		sunxi_mmc_tuning_set_fifo_bypass(run->sdhci, scan->fifo_bypass);
		scan->set_delay(run->sdhci, delay);
		run->pass[delay] = scan->probe(run->sdhci, scan->probe_arg) == 0;
	}
}

/**
 * @brief Probe a delay point by reading back the tuning pattern
 * @details Reads the tuning pattern from the reserved card LBA and compares
 *          it with the expected data to determine whether the current delay
 *          setting yields a valid transfer.
 * @param sdhci Pointer to the SDHCI controller structure
 * @param arg Pointer to the tuning pattern structure
 * @return 0 if the pattern matches, -1 otherwise
 */
static int sunxi_mmc_tuning_probe_pattern(sunxi_sdhci_t *sdhci, const void *arg)
{
	const struct sunxi_mmc_tuning_pattern *pattern = arg;
	uint32_t ret;

	if (sdhci == NULL || pattern == NULL || pattern->data == NULL || pattern->readback == NULL ||
		pattern->blocks == 0U)
		return -1;

	ret = sunxi_mmc_blk_read(sdhci, pattern->readback, SUNXI_MMC_TUNING_LBA, pattern->blocks);
	if (ret != pattern->blocks) {
		sunxi_mmc_tuning_recover_data_transfer(sdhci);
		return -1;
	}

	return memcmp(pattern->data, pattern->readback,
		pattern->blocks * SUNXI_MMC_TUNING_BLOCK_SIZE) == 0 ? 0 : -1;
}

/**
 * @brief Probe a delay point using the HS400 command path
 * @details Sends the SEND_STATUS command and checks that the card is ready
 *          for data without any error status bits set.
 * @param sdhci Pointer to the SDHCI controller structure
 * @param arg Unused probe argument
 * @return 0 if the command succeeds and the card is ready, -1 otherwise
 */
static int sunxi_mmc_tuning_probe_hs400_command(sunxi_sdhci_t *sdhci, const void *arg)
{
	mmc_cmd_t cmd = { 0 };

	(void)arg;
	if (sdhci == NULL)
		return -1;

	cmd.cmdidx = MMC_CMD_SEND_STATUS;
	cmd.resp_type = MMC_RSP_R1;
	cmd.cmdarg = sdhci->mmc.rca << 16;
	if (sunxi_sdhci_xfer_timeout(sdhci, &cmd, NULL, SUNXI_MMC_TUNING_TIMEOUT_US)) {
		/* Command errors reset the active timing state. */
		sunxi_mmc_tuning_restore_link(sdhci);
		return -1;
	}

	return (!(cmd.response[0] & MMC_STATUS_RDY_FOR_DATA) || (cmd.response[0] & MMC_STATUS_MASK)) ? -1 : 0;
}

/**
 * @brief Select the best delay from the pass results
 * @details Finds the longest contiguous run of passing delay points and
 *          returns the midpoint of that window. Returns an invalid marker if
 *          no window is at least as large as the minimum required size.
 * @param pass Array of pass/fail results for each delay point
 * @return The selected delay value, or SUNXI_MMC_TUNING_INVALID
 */
static uint32_t sunxi_mmc_tuning_select(const uint8_t *pass)
{
	uint32_t best_start = 0U;
	uint32_t best_length = 0U;

	if (pass == NULL)
		return SUNXI_MMC_TUNING_INVALID;

	for (uint32_t start = 0U; start < SUNXI_MMC_TUNING_POINTS; ++start) {
		uint32_t length = 0U;

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

/**
 * @brief Store the selected sample delay in the tuning table
 * @details Writes the selected delay into the speed-mode/frequency entry of
 *          the tune_sdly sample table, preserving other entries.
 * @param mmc Pointer to the MMC structure
 * @param speed_mode Speed mode the sample was tuned for
 * @param freq_id Frequency ID the sample was tuned for
 * @param selected Selected delay value
 */
static void sunxi_mmc_tuning_store_sample(mmc_t *mmc, uint32_t speed_mode, uint32_t freq_id, uint32_t selected)
{
	uint32_t index;
	uint32_t shift;
	uint32_t value;

	if (mmc == NULL || selected >= SUNXI_MMC_TUNING_POINTS || freq_id >= MMC_MAX_CLK_FREQ_NUM ||
		speed_mode >= MMC_MAX_SPD_MD_NUM)
		return;

	index = speed_mode * 2U + freq_id / 4U;
	shift = (freq_id % 4U) * 8U;
	value = mmc->tune_sdly.tm4_smx_fx[index];
	value &= ~(0xffU << shift);
	mmc->tune_sdly.tm4_smx_fx[index] = value | (selected << shift);
}

/**
 * @brief Prepare the card for tuning by writing and verifying the pattern
 * @details At a safe clock, writes the tuning pattern to the reserved card
 *          LBA, reads it back, and verifies it matches. Skips the operation
 *          when the card was already prepared for this host and bus width,
 *          and caches the prepared state on success.
 * @param sdhci Pointer to the SDHCI controller structure
 * @param pattern Pointer to the tuning pattern structure
 * @return 0 on success, -1 on failure
 */
static int sunxi_mmc_tuning_prepare_card(sunxi_sdhci_t *sdhci, const struct sunxi_mmc_tuning_pattern *pattern)
{
	mmc_t *mmc;
	uint32_t original_clock;
	uint32_t safe_clock;
	uint32_t ret;
	int result = -1;
	bool transfer_failed = false;
	bool card_prepared = false;

	if (sdhci == NULL || pattern == NULL || pattern->data == NULL || pattern->readback == NULL ||
		pattern->blocks == 0U)
		return -1;
	mmc = &sdhci->mmc;
	if (SUNXI_MMC_TUNING_LBA > mmc->lba || pattern->blocks > mmc->lba - SUNXI_MMC_TUNING_LBA)
		return -1;
	if (sunxi_mmc_tuning_patterns.host == sdhci && sunxi_mmc_tuning_patterns.bus_width == mmc->bus_width)
		return 0;

	original_clock = mmc->clock;
	safe_clock = original_clock > SUNXI_MMC_TUNING_SAFE_CLOCK ? SUNXI_MMC_TUNING_SAFE_CLOCK : original_clock;
	if (safe_clock < mmc->f_min)
		safe_clock = mmc->f_min;
	sunxi_mmc_hs_set_clock(sdhci, safe_clock);
	if (sdhci->mmc_host.fatal_err)
		goto restore;

	ret = sunxi_mmc_blk_write(sdhci, pattern->data, SUNXI_MMC_TUNING_LBA, pattern->blocks);
	if (ret != pattern->blocks) {
		transfer_failed = true;
		goto restore;
	}
	ret = sunxi_mmc_blk_read(sdhci, pattern->readback, SUNXI_MMC_TUNING_LBA, pattern->blocks);
	if (ret != pattern->blocks) {
		transfer_failed = true;
		goto restore;
	}
	if (memcmp(pattern->data, pattern->readback,
		pattern->blocks * SUNXI_MMC_TUNING_BLOCK_SIZE) != 0)
		goto restore;

	card_prepared = true;
	result = 0;

restore:
	if (transfer_failed)
		sunxi_mmc_tuning_recover_data_transfer(sdhci);
	sunxi_mmc_hs_set_clock(sdhci, original_clock);
	if (sdhci->mmc_host.fatal_err)
		result = -1;
	if (card_prepared && result == 0) {
		sunxi_mmc_tuning_patterns.host = sdhci;
		sunxi_mmc_tuning_patterns.bus_width = mmc->bus_width;
	}
	return result;
}

/**
 * @brief Check whether the card is already prepared for tuning
 * @details Returns true when the cached prepared state matches the given host
 *          and its current bus width.
 * @param sdhci Pointer to the SDHCI controller structure
 * @return True if the card is prepared for this host and bus width
 */
static bool sunxi_mmc_tuning_card_ready(const sunxi_sdhci_t *sdhci)
{
	return sdhci != NULL && sunxi_mmc_tuning_patterns.host == sdhci &&
		sunxi_mmc_tuning_patterns.bus_width == sdhci->mmc.bus_width;
}

#ifdef CONFIG_DRIVER_MMC_SHOW_TRAINING
/**
 * @brief Dump a text chart of the tuning pass results
 * @details Prints an ASCII chart showing pass/fail results for each delay
 *          point and marks the selected delay. Debug helper compiled only
 *          when CONFIG_DRIVER_MMC_SHOW_TRAINING is enabled.
 * @param name Name of the tuning mode for the chart title
 * @param pass Array of pass/fail results for each delay point
 * @param selected The selected delay to mark
 */
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
	pr_debug("result |%s|\n", samples);
	if (selected >= SUNXI_MMC_TUNING_POINTS)
		return;

	memset(selected_line, ' ', SUNXI_MMC_TUNING_POINTS);
	selected_line[selected] = '^';
	selected_line[SUNXI_MMC_TUNING_POINTS] = '\0';
	pr_debug("select |%s| delay=%u\n", selected_line, selected);
}
#endif

/**
 * @brief Return the display name of a tuning mode
 * @details Maps a tuning mode enum value to a human-readable string.
 * @param mode The tuning mode
 * @return The mode name string
 */
static const char *sunxi_mmc_tuning_mode_name(sunxi_mmc_tuning_mode_t mode)
{
	switch (mode) {
	case SUNXI_MMC_TUNING_HS200:
		return "HS200/SDR104";
	case SUNXI_MMC_TUNING_HS400_COMMAND:
		return "HS400 command";
	default:
		return "HS400 data";
	}
}

/**
 * @brief Return the MMC bus width in bits
 * @details Converts the stored bus width to a bit count, returning 8, 4, or 1.
 * @param mmc Pointer to the MMC structure
 * @return The bus width in bits
 */
static uint32_t sunxi_mmc_tuning_bus_width(const mmc_t *mmc)
{
	if (mmc == NULL)
		return 0U;
	if (mmc->bus_width == SMHC_WIDTH_8BIT)
		return 8U;
	if (mmc->bus_width == SMHC_WIDTH_4BIT)
		return 4U;
	return 1U;
}

/**
 * @brief Print the tuning run result
 * @details Logs the frequency, clock, bus width, number of points, and the
 *          selected delay, optionally dumping the pass chart and the stored
 *          sample/delay values for the tuning mode.
 * @param mode The tuning mode that was executed
 * @param run Pointer to the completed tuning run
 * @param pattern_blocks Number of pattern blocks used (HS200 data tuning)
 */
static void sunxi_mmc_tuning_print_result(sunxi_mmc_tuning_mode_t mode,
	const struct sunxi_mmc_tuning_run *run, uint32_t pattern_blocks)
{
	const char *name = sunxi_mmc_tuning_mode_name(mode);
	const mmc_t *mmc = run->mmc;

	if (run->selected == SUNXI_MMC_TUNING_INVALID)
		pr_info("%s: freq=%u clock=%uHz bus=%ubit points=%u selected=invalid\n", name, run->freq_id,
			mmc->clock, sunxi_mmc_tuning_bus_width(mmc), SUNXI_MMC_TUNING_POINTS);
	else
		pr_info("%s: freq=%u clock=%uHz bus=%ubit points=%u selected=%u\n", name, run->freq_id,
			mmc->clock, sunxi_mmc_tuning_bus_width(mmc), SUNXI_MMC_TUNING_POINTS, run->selected);

#ifdef CONFIG_DRIVER_MMC_SHOW_TRAINING
	sunxi_mmc_tuning_dump_chart(name, run->pass, run->selected);
#endif

	if (mode == SUNXI_MMC_TUNING_HS200) {
		pr_info("%s: pattern_lba=%u blocks=%u smx_fx=0x%08x 0x%08x\n", name, SUNXI_MMC_TUNING_LBA,
			pattern_blocks, mmc->tune_sdly.tm4_smx_fx[MMC_HS200_SDR104 * 2U],
			mmc->tune_sdly.tm4_smx_fx[MMC_HS200_SDR104 * 2U + 1U]);
	} else if (mode == SUNXI_MMC_TUNING_HS400_DATA) {
		if (run->selected == SUNXI_MMC_TUNING_INVALID)
			pr_info("%s: dsdly=0x%02x%02x%02x%02x%02x%02x selected=invalid\n", name,
				mmc->tune_sdly.tm4_dsdly[0], mmc->tune_sdly.tm4_dsdly[1], mmc->tune_sdly.tm4_dsdly[2],
				mmc->tune_sdly.tm4_dsdly[3], mmc->tune_sdly.tm4_dsdly[4], mmc->tune_sdly.tm4_dsdly[5]);
		else
			pr_info("%s: dsdly=0x%02x%02x%02x%02x%02x%02x selected=%u\n", name,
				mmc->tune_sdly.tm4_dsdly[0], mmc->tune_sdly.tm4_dsdly[1], mmc->tune_sdly.tm4_dsdly[2],
				mmc->tune_sdly.tm4_dsdly[3], mmc->tune_sdly.tm4_dsdly[4], mmc->tune_sdly.tm4_dsdly[5], run->selected);
	} else {
		pr_info("%s: smx_fx=0x%08x 0x%08x\n", name, mmc->tune_sdly.tm4_smx_fx[MMC_HS400 * 2U],
			mmc->tune_sdly.tm4_smx_fx[MMC_HS400 * 2U + 1U]);
	}
}

/**
 * @brief Check whether the host is valid for the given tuning mode
 * @details Verifies the host is SMHC2 with timing mode 4 and that its current
 *          speed mode and bus width match the requested tuning parameters.
 * @param sdhci Pointer to the SDHCI controller structure
 * @param speed_mode Expected speed mode
 * @param width Expected bus width
 * @return True if the host matches, false otherwise
 */
static bool sunxi_mmc_tuning_host_valid(const sunxi_sdhci_t *sdhci, uint32_t speed_mode, uint32_t width)
{
	return sdhci != NULL && sdhci->id == MMC_CONTROLLER_2 &&
		sdhci->mmc_host.timing_mode == SUNXI_MMC_TIMING_MODE_4 && sdhci->mmc.speed_mode == speed_mode &&
		sdhci->mmc.bus_width == width;
}

/**
 * @brief Execute HS400 command tuning
 * @details Scans the sampling delay points using the HS400 command path,
 *          selects the best delay, programs it, and stores it in the tuning
 *          table. Restricted to SMHC2 in timing mode 4 with an 8-bit HS400
 *          bus.
 * @param sdhci Pointer to the SDHCI controller structure
 * @return 0 on success, -1 on failure
 */
int sunxi_mmc_execute_hs400_command_tuning(sunxi_sdhci_t *sdhci)
{
	struct sunxi_mmc_tuning_run run;
	struct sunxi_mmc_tuning_scan scan = {
		.set_delay = sunxi_mmc_tuning_set_sample,
		.probe = sunxi_mmc_tuning_probe_hs400_command,
		.fifo_bypass = true,
	};
	int result = -1;

	if (!sunxi_mmc_tuning_host_valid(sdhci, MMC_HS400, SMHC_WIDTH_8BIT) ||
		!sunxi_mmc_tuning_begin(&run, sdhci))
		return -1;

	run.freq_id = sunxi_mmc_tuning_freq_id(run.mmc->clock);
	sunxi_mmc_tuning_scan(&run, &scan);
	run.selected = sunxi_mmc_tuning_select(run.pass);
	if (run.selected != SUNXI_MMC_TUNING_INVALID) {
		sunxi_mmc_tuning_set_sample(sdhci, run.selected);
		sunxi_mmc_tuning_store_sample(run.mmc, MMC_HS400, run.freq_id, run.selected);
		result = 0;
	}
	sunxi_mmc_tuning_print_result(SUNXI_MMC_TUNING_HS400_COMMAND, &run, 0U);
	sunxi_mmc_tuning_end(&run);
	return result;
}

/**
 * @brief Execute HS200/SDR104 data tuning
 * @details Builds a tuning pattern, prepares the card, scans the sampling
 *          delay points by reading back the pattern, selects the best delay,
 *          and stores it in the tuning table.
 * @param sdhci Pointer to the SDHCI controller structure
 * @return 0 on success, -1 on failure
 */
int sunxi_mmc_execute_tuning(sunxi_sdhci_t *sdhci)
{
	struct sunxi_mmc_tuning_run run;
	struct sunxi_mmc_tuning_pattern pattern = { 0 };
	struct sunxi_mmc_tuning_scan scan = {
		.set_delay = sunxi_mmc_tuning_set_sample,
		.probe = sunxi_mmc_tuning_probe_pattern,
		.fifo_bypass = false,
	};
	int result = -1;

	if (sdhci == NULL || (sdhci->mmc.bus_width != SMHC_WIDTH_4BIT && sdhci->mmc.bus_width != SMHC_WIDTH_8BIT) ||
		!sunxi_mmc_tuning_host_valid(sdhci, MMC_HS200_SDR104, sdhci->mmc.bus_width) ||
		!sunxi_mmc_tuning_begin(&run, sdhci))
		return -1;

	if (sunxi_mmc_tuning_get_pattern(sdhci, &pattern) || sunxi_mmc_tuning_prepare_card(sdhci, &pattern))
		goto out;

	run.freq_id = sunxi_mmc_tuning_freq_id(run.mmc->clock);
	scan.probe_arg = &pattern;
	sunxi_mmc_tuning_scan(&run, &scan);
	run.selected = sunxi_mmc_tuning_select(run.pass);
	if (run.selected != SUNXI_MMC_TUNING_INVALID) {
		sunxi_mmc_tuning_set_sample(sdhci, run.selected);
		sunxi_mmc_tuning_store_sample(run.mmc, MMC_HS200_SDR104, run.freq_id, run.selected);
		result = 0;
	}
	sunxi_mmc_tuning_print_result(SUNXI_MMC_TUNING_HS200, &run, pattern.blocks);

out:
	sunxi_mmc_tuning_release_pattern(&pattern);
	sunxi_mmc_tuning_end(&run);
	return result;
}

/**
 * @brief Execute HS400 data tuning
 * @details Scans the data-strobe delay points by reading back the tuning
 *          pattern, selects the best delay, and stores it in the HS400
 *          data-strobe delay table.
 * @param sdhci Pointer to the SDHCI controller structure
 * @return 0 on success, -1 on failure
 */
int sunxi_mmc_execute_hs400_tuning(sunxi_sdhci_t *sdhci)
{
	struct sunxi_mmc_tuning_run run;
	struct sunxi_mmc_tuning_pattern pattern = { 0 };
	struct sunxi_mmc_tuning_scan scan = {
		.set_delay = sunxi_mmc_tuning_set_data_strobe,
		.probe = sunxi_mmc_tuning_probe_pattern,
		.fifo_bypass = false,
	};
	int result = -1;

	if (!sunxi_mmc_tuning_host_valid(sdhci, MMC_HS400, SMHC_WIDTH_8BIT) ||
		sdhci->mmc.capacity < SUNXI_MMC_TUNING_BLOCK_SIZE || !sunxi_mmc_tuning_begin(&run, sdhci))
		return -1;
	if (sunxi_mmc_tuning_get_pattern(sdhci, &pattern))
		goto out;
	if (!sunxi_mmc_tuning_card_ready(sdhci))
		goto out;

	run.freq_id = sunxi_mmc_tuning_freq_id(run.mmc->clock);
	scan.probe_arg = &pattern;
	sunxi_mmc_tuning_scan(&run, &scan);
	run.selected = sunxi_mmc_tuning_select(run.pass);
	if (run.selected != SUNXI_MMC_TUNING_INVALID) {
		sunxi_mmc_tuning_set_data_strobe(sdhci, run.selected);
		run.mmc->tune_sdly.tm4_dsdly[run.freq_id] = (uint8_t)run.selected;
		result = 0;
	}
	sunxi_mmc_tuning_print_result(SUNXI_MMC_TUNING_HS400_DATA, &run, pattern.blocks);

out:
	sunxi_mmc_tuning_release_pattern(&pattern);
	sunxi_mmc_tuning_end(&run);
	return result;
}
