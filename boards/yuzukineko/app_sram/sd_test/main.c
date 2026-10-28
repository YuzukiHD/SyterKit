/* SPDX-License-Identifier: GPL-2.0+ */

/*
 * SD card scan test: bring up SMHC0, identify the card and read the first
 * sectors. Used as the reference for the Linux and SyterOS SMHC drivers.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <types.h>

#include <common.h>
#include <driver.h>
#include <io.h>
#include <log.h>

#include <dt-bindings/soc/sun252iw2.h>
#include <drivers/clk/clk.h>
#include <drivers/mmc/sdcard.h>
#include <drivers/mmc/sdhci.h>
#include <drivers/serial/serial.h>
#include <dt-compatible/mmc-dt.h>

#define PLL_PERI_REG 0x20
#define SD_TEST_SECTORS 4U

static uint8_t sector_buf[SD_TEST_SECTORS * 512U] __attribute__((aligned(64)));

static void dump_regs(const char *what, uintptr_t base, uint32_t first, uint32_t last)
{
	pr_info("%s:\n", what);
	for (uint32_t off = first; off <= last; off += 4U)
		pr_info("  [%08x] = %08x\n", (unsigned)(base + off), (unsigned)readl(base + off));
}

static void dump_bytes(const uint8_t *p, uint32_t len)
{
	for (uint32_t i = 0; i < len; i += 16U) {
		printk(LOG_LEVEL_INFO, "  %04x:", (unsigned)i);
		for (uint32_t j = 0; j < 16U && i + j < len; j++)
			printk(LOG_LEVEL_INFO, " %02x", p[i + j]);
		printk(LOG_LEVEL_INFO, "\n");
	}
}

int main(void)
{
	sunxi_sdhci_t sdhci = { 0 };
	sdmmc_pdata_t card = { 0 };

	if (sunxi_serial_init_stdout() != DRIVER_OK)
		return -1;

	show_banner();
	sunxi_clk_init();

	pr_info("PLL_PERI = %08x, SMHC0 clk = %08x, bus gate/reset = %08x\n",
		(unsigned)readl(SUNXI_CCU_BASE + PLL_PERI_REG),
		(unsigned)readl(SUNXI_CCU_BASE + 0x830),
		(unsigned)readl(SUNXI_CCU_BASE + 0x84c));

	if (sunxi_sdhci_dt_read_alias(&sdhci, "mmc0") != DRIVER_OK) {
		pr_err("SMHC: invalid devicetree configuration\n");
		return -1;
	}

	if (sunxi_sdhci_init(&sdhci) != 0) {
		pr_err("SMHC: %s controller init failed\n", sdhci.name);
		return -1;
	}
	pr_info("SMHC: %s controller initialized\n", sdhci.name);

	if (sdmmc_init(&card, &sdhci) != 0) {
		pr_err("SMHC: card init failed\n");
		dump_regs("SMHC0", SUNXI_SMHC0_BASE, 0x00, 0x60);
		return -1;
	}

	const mmc_t *mmc = &sdhci.mmc;

	pr_info("SD: ocr %08x, %s capacity, rca %04x\n", (unsigned)mmc->ocr,
		mmc->high_capacity ? "high" : "standard", (unsigned)mmc->rca);
	pr_info("SD: cid %08x %08x %08x %08x\n", (unsigned)mmc->cid[0], (unsigned)mmc->cid[1],
		(unsigned)mmc->cid[2], (unsigned)mmc->cid[3]);
	pr_info("SD: csd %08x %08x %08x %08x\n", (unsigned)mmc->csd[0], (unsigned)mmc->csd[1],
		(unsigned)mmc->csd[2], (unsigned)mmc->csd[3]);
	pr_info("SD: scr %08x %08x, %u blocks, %u MiB, clock %u Hz, %u bit\n", (unsigned)mmc->scr[0],
		(unsigned)mmc->scr[1], (unsigned)mmc->lba, (unsigned)(mmc->capacity >> 20),
		(unsigned)mmc->clock, (unsigned)mmc->bus_width);

	memset(sector_buf, 0xa5, sizeof(sector_buf));
	if (sdmmc_blk_read(&card, sector_buf, 0, SD_TEST_SECTORS) != SD_TEST_SECTORS) {
		pr_err("SD: read of sectors 0..%u failed\n", SD_TEST_SECTORS - 1U);
		return -1;
	}
	pr_info("SD: sector 0 (boot signature %02x%02x, FAT expects 55aa):\n", sector_buf[510], sector_buf[511]);
	dump_bytes(sector_buf, 64);

	dump_regs("SMHC0", SUNXI_SMHC0_BASE, 0x00, 0x60);
	pr_info("SD: scan done\n");

	for (;;)
		__asm__ __volatile__("wfi");
}
