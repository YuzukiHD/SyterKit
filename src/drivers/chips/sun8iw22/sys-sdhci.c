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
	uint32_t src, sclk_hz, div, n, m;

	if (clk_hz <= 4000000) {
		src = 0;
		sclk_hz = 24000000;
	} else {
		src = 1;
		sclk_hz = (sdhci->id == 2) ? 800000000 : 400000000;
	}
	div = sclk_hz / clk_hz;
	if (sclk_hz % clk_hz)
		div += 1;

	for (n = 1; n <= 32; n++) {
		for (m = n; m <= 32; m++) {
			if (n * m == div) {
				printk_debug("SMHC: div=%d n=%d m=%d\n", div, n, m);
				clk.factor_n = n - 1;
				clk.factor_m = m - 1;
				clk.clk_sel = src;
				goto freq_out;
			}
		}
	}

	printk_warning("SMHC: wrong clock source, div=%d n = %d m=%d\n", div, n, m);
	return -1;

freq_out:
	// Configure the clock register value
	reg_val = (clk.clk_sel << 24) | (clk.factor_n << 16) | clk.factor_m;
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
	clk.factor_n = (reg_val >> 16) & 0x3;
	clk.clk_sel = (reg_val >> 24) & 0x3;

	// Calculate the current clock frequency based on the source and divider values
	switch (clk.clk_sel) {
		case 0:
			clk_hz = 24000000;
			break;
		case 1:
			clk_hz = (sdhci->id == 2) ? 800000000 : 400000000;
			break;
		default:
			printk_debug("SMHC: wrong clock source %u\n", clk.clk_sel);
			break;
	}

	// Calculate the actual clock frequency based on the divider values
	return clk_hz / (clk.factor_n + 1) / (clk.factor_m + 1);
}