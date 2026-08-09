/* SPDX-License-Identifier:	GPL-2.0+ */

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>
#include <timer.h>

#include <drivers/gpio.h>

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
int sunxi_sdhci_set_mclk(sunxi_sdhci_t *sdhci, uint32_t clk_hz) {
	uint32_t reg_val = 0x0;
	uint32_t src, sclk_hz, div, n, m;

	if (sdhci == NULL || clk_hz == 0U)
		return -1;
	sunxi_sdhci_clk_t clk = sdhci->sdhci_clk;

	if (clk_hz <= 4000000) {
		src = 0;
	} else {
		src = clk.default_clk_sel;
	}
	sclk_hz = sunxi_sdhci_clk_source_rate(&clk, src);
	if (sclk_hz == 0U) {
		printk_debug("SMHC: unsupported clock source %u\n", src);
		return -1;
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
				goto freq_out;
			}
		}
	}

	printk_warning("SMHC: wrong clock source, div=%d n = %d m=%d\n", div, n, m);
	return -1;

freq_out:
	// Configure the clock register value
	reg_val = (src << 24) |
		  (clk.factor_n << clk.reg_factor_n_offset) |
		  (clk.factor_m << clk.reg_factor_m_offset);
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
	uint32_t clk_hz = 0;
	uint32_t reg_val = 0x0;
	uint32_t source;

	if (sdhci == NULL)
		return 0U;
	sunxi_sdhci_clk_t clk = sdhci->sdhci_clk;

	// Read the clock register value
	reg_val = readl(clk.reg_base);

	// Extract the divider values and clock source from the register value
	clk.factor_m = (reg_val >> clk.reg_factor_m_offset) & 0xfU;
	clk.factor_n = (reg_val >> clk.reg_factor_n_offset) & 0x3U;
	source = (reg_val >> 24) & 0x3U;

	clk_hz = sunxi_sdhci_clk_source_rate(&clk, source);
	if (clk_hz == 0U) {
		printk_debug("SMHC: unsupported clock source %u\n", source);
		return 0U;
	}

	// Calculate the actual clock frequency based on the divider values
	return clk_hz / (clk.factor_n + 1) / (clk.factor_m + 1);
}
