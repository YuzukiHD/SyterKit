/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "syter_test.h"

#include "../../../drivers/mmc/tuning.c"

static uint8_t heap[24576];
static uint8_t card[SUNXI_MMC_TUNING_MAX_BLOCKS * SUNXI_MMC_TUNING_BLOCK_SIZE];
static uint32_t card_blocks;
static uint32_t write_count;
static uint32_t read_count;

void printk(int level, const char *fmt, ...)
{
	(void)level;
	(void)fmt;
}

uint32_t sunxi_mmc_blk_read(void *sdhci, void *dst, uint32_t start, uint32_t blocks)
{
	(void)sdhci;
	if (dst == NULL || start != SUNXI_MMC_TUNING_LBA || blocks == 0U || blocks > card_blocks)
		return 0U;
	memcpy(dst, card, blocks * SUNXI_MMC_TUNING_BLOCK_SIZE);
	read_count++;
	return blocks;
}

uint32_t sunxi_mmc_blk_write(void *sdhci, void *src, uint32_t start, uint32_t blocks)
{
	(void)sdhci;
	if (src == NULL || start != SUNXI_MMC_TUNING_LBA || blocks == 0U || blocks > SUNXI_MMC_TUNING_MAX_BLOCKS)
		return 0U;
	memcpy(card, src, blocks * SUNXI_MMC_TUNING_BLOCK_SIZE);
	card_blocks = blocks;
	write_count++;
	return blocks;
}

void sunxi_mmc_hs_set_clock(sunxi_sdhci_t *sdhci, uint32_t clock)
{
	sdhci->mmc.clock = clock;
}

void sunxi_sdhci_set_ios(sunxi_sdhci_t *sdhci)
{
	(void)sdhci;
}

int sunxi_sdhci_xfer_timeout(sunxi_sdhci_t *sdhci, mmc_cmd_t *cmd, mmc_data_t *data, uint32_t timeout_us)
{
	(void)sdhci;
	(void)cmd;
	(void)data;
	(void)timeout_us;
	return 0;
}

static void init_host(sunxi_sdhci_t *sdhci, sdhci_reg_t *registers)
{
	memset(sdhci, 0, sizeof(*sdhci));
	memset(registers, 0, sizeof(*registers));
	sdhci->id = MMC_CONTROLLER_2;
	sdhci->sample_fifo_bypass = true;
	sdhci->mmc_host.reg = registers;
	sdhci->mmc_host.timing_mode = SUNXI_MMC_TIMING_MODE_4;
	sdhci->mmc.bus_width = SMHC_WIDTH_8BIT;
	sdhci->mmc.speed_mode = MMC_HS200_SDR104;
	sdhci->mmc.clock = 150000000U;
	sdhci->mmc.f_min = 400000U;
	sdhci->mmc.capacity = 64U * 1024U * 1024U;
	sdhci->mmc.lba = (uint32_t)(sdhci->mmc.capacity / SUNXI_MMC_TUNING_BLOCK_SIZE);
}

static void check_workspace(sunxi_sdhci_t *sdhci)
{
	struct sunxi_mmc_tuning_pattern pattern = { 0 };
	uint8_t first_byte;

	sdhci->mmc.bus_width = SMHC_WIDTH_4BIT;
	TEST_EQ(0, sunxi_mmc_tuning_get_pattern(sdhci, &pattern));
	TEST_EQ(6U, pattern.blocks);
	TEST_EQ(0U, (uintptr_t)pattern.data & (SUNXI_MMC_TUNING_ALIGNMENT - 1U));
	TEST_EQ(0U, (uintptr_t)pattern.readback & (SUNXI_MMC_TUNING_ALIGNMENT - 1U));
	sunxi_mmc_tuning_release_pattern(&pattern);

	sdhci->mmc.bus_width = SMHC_WIDTH_8BIT;
	TEST_EQ(0, sunxi_mmc_tuning_get_pattern(sdhci, &pattern));
	TEST_EQ(SUNXI_MMC_TUNING_MAX_BLOCKS, pattern.blocks);
	TEST_EQ(0U, (uintptr_t)pattern.data & (SUNXI_MMC_TUNING_ALIGNMENT - 1U));
	TEST_EQ(0U, (uintptr_t)pattern.readback & (SUNXI_MMC_TUNING_ALIGNMENT - 1U));
	TEST_ASSERT(memcmp(pattern.data, pattern.data + SUNXI_MMC_TUNING_BLOCK_SIZE, 64U) != 0);
	first_byte = pattern.data[0];
	sunxi_mmc_tuning_release_pattern(&pattern);

	TEST_EQ(0, sunxi_mmc_tuning_get_pattern(sdhci, &pattern));
	TEST_EQ(first_byte, pattern.data[0]);
	sunxi_mmc_tuning_release_pattern(&pattern);
}

void test_case_main(const char *case_dir)
{
	sunxi_sdhci_t sdhci;
	sdhci_reg_t registers;
	void *large_allocation;

	(void)case_dir;
	TEST_EQ(0, malloc_init((uintptr_t)heap, sizeof(heap)));
	init_host(&sdhci, &registers);
	check_workspace(&sdhci);

	TEST_EQ(0, sunxi_mmc_execute_tuning(&sdhci));
	TEST_EQ(1U, write_count);
	TEST_EQ(SUNXI_MMC_TUNING_MAX_BLOCKS, card_blocks);
	TEST_EQ(65U, read_count);
	TEST_EQ(31U, sdhci.mmc.tune_sdly.tm4_smx_fx[MMC_HS200_SDR104 * 2U + MMC_CLK_150M / 4U]);

	sdhci.mmc.speed_mode = MMC_HS400;
	TEST_EQ(0, sunxi_mmc_execute_hs400_tuning(&sdhci));
	TEST_EQ(129U, read_count);
	TEST_EQ(31U, sdhci.mmc.tune_sdly.tm4_dsdly[MMC_CLK_150M]);

	large_allocation = malloc(20000U);
	TEST_ASSERT(large_allocation != NULL);
	free(large_allocation);
}
