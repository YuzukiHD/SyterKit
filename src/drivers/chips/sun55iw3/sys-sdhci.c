/* SPDX-License-Identifier:	GPL-2.0+ */

#include <barrier.h>
#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>
#include <timer.h>

#include <sys-clk.h>
#include <sys-gpio.h>

#include <mmc/sys-mmc.h>
#include <mmc/sys-sdhci.h>

/**
 * @brief Set the SDHC controller's clock frequency.
 * 
 * This function sets the clock frequency for the specified SDHC controller.
 * 
 * @param sdhci Pointer to the SDHC controller structure.
 * @param clk_hz Desired clock frequency in Hertz.
 * @return Returns 0 on success, -1 on failure.
 */
int sunxi_sdhci_set_mclk(sunxi_sdhci_t *sdhci, uint32_t clk_hz) {
	uint32_t reg_val = 0x0;
	mmc_t *mmc = sdhci->mmc;
	sunxi_sdhci_clk_t clk = sdhci->sdhci_clk;

	// Determine the clock source based on the requested frequency
	if (clk_hz <= 4000000) {
		clk.clk_sel = 0;// SCLK = 24000000 OSC CLK
	} else {
		clk.clk_sel = 2;// SCLK = AHB PLL
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
			clk.clk_sel = 1;// PERI0_800M
			clk.factor_n = 0;
			clk.factor_m = 3;
			break;
		case 100000001 ... 150000000:
			clk.clk_sel = 1;// PERI0_800M
			clk.factor_n = 0;
			clk.factor_m = 2;
			break;
		case 150000001 ... 200000000:
			clk.clk_sel = 1;// PERI0_800M
			clk.factor_n = 0;
			clk.factor_m = 1;
			break;
		default:
			clk.factor_n = (sdhci->id == 2) ? 1 : 0;
			clk.factor_m = 2;
			printk_debug("SMHC: requested frequency does not match: freq=%d, default to 52000000\n", clk_hz);
			break;
	}

	// Configure the clock register value
	reg_val = (clk.clk_sel << 24) | (clk.factor_n << 8) | clk.factor_m;
	writel(reg_val, clk.reg_base);

	return 0;
}

/**
 * @brief Get the current clock frequency of the SDHC controller.
 * 
 * This function retrieves the current clock frequency of the specified SDHC controller.
 * 
 * @param sdhci Pointer to the SDHC controller structure.
 * @return Current clock frequency in Hertz.
 */
uint32_t sunxi_sdhci_get_mclk(sunxi_sdhci_t *sdhci) {
	uint32_t clk_hz;
	uint32_t reg_val = 0x0;
	sunxi_sdhci_clk_t clk = sdhci->sdhci_clk;

	// Read the clock register value
	reg_val = readl(clk.reg_base);

	// Extract the divider values and clock source from the register value
	clk.factor_m = reg_val & 0xf;
	clk.factor_n = (reg_val >> 8) & 0x3;
	clk.clk_sel = (reg_val >> 24) & 0x3;

	// Calculate the current clock frequency based on the source and divider values
	switch (clk.clk_sel) {
		case 0:
			clk_hz = 24000000;
			break;
		case 1:
		case 3:
			clk_hz = (sdhci->id == 2) ? 800000000 : 400000000;
			break;
		case 2:
		case 4:
			clk_hz = (sdhci->id == 2) ? 600000000 : 300000000;
			break;
		default:
			printk_debug("SMHC: wrong clock source %u\n", clk.clk_sel);
			break;
	}

	// Calculate the actual clock frequency based on the divider values
	return clk_hz / (clk.factor_n + 1) / (clk.factor_m + 1);
}