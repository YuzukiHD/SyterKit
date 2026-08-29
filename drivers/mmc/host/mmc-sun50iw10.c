/* SPDX-License-Identifier:	GPL-2.0+ */

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
	uint32_t reg_val = 0x0, sclk_hz = 0x0, div = 0x0;
	uint32_t source;

	if (sdhci == NULL || clk_hz == 0U)
		return -1;
	sunxi_sdhci_clk_t clk = sdhci->sdhci_clk;

	source = clk_hz <= 4000000U ? 0U : clk.default_clk_sel;
	sclk_hz = sunxi_sdhci_clk_source_rate(&clk, source);
	if (sclk_hz == 0U) {
		pr_debug("SMHC: unsupported clock source %u\n", source);
		return -1;
	}

	div = (2 * sclk_hz + clk_hz) / (2 * clk_hz);
	div = (div == 0) ? 1 : div;

	if (div > 128) {
		clk.factor_m = 1;
		clk.factor_n = 0;
		pr_warn("SMHC: Source clk is too high.\n");
	} else if (div > 64) {
		clk.factor_n = 3;
		clk.factor_m = div >> 3;
	} else if (div > 32) {
		clk.factor_n = 2;
		clk.factor_m = div >> 2;
	} else if (div > 16) {
		clk.factor_n = 1;
		clk.factor_m = div >> 1;
	} else {
		clk.factor_n = 0;
		clk.factor_m = div;
	}

	reg_val = BIT(31) | (source << 24) | ((clk.factor_n) << clk.reg_factor_n_offset) | ((clk.factor_m - 1) << clk.reg_factor_m_offset);

	writel(reg_val, clk.reg_base);

	pr_trace("SMHC: sdhci%d clk want %uHz parent %uHz, mclk=0x%08x clk_sel=%u, div=%u, n=%u, m=%u\n", sdhci->id, clk_hz, sclk_hz, readl(sdhci->sdhci_clk.reg_base), source,
		     div, clk.factor_n, clk.factor_m);
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
uint32_t sunxi_sdhci_get_mclk(sunxi_sdhci_t *sdhci)
{
	uint32_t clk_hz = 0x0;
	uint32_t reg_val = 0x0;
	uint32_t source;

	if (sdhci == NULL)
		return 0U;
	sunxi_sdhci_clk_t clk = sdhci->sdhci_clk;

	// Read the clock register value
	reg_val = readl(clk.reg_base);

	// Extract the divider values and clock source from the register value
	clk.factor_m = (reg_val >> clk.reg_factor_m_offset) & 0xf;
	clk.factor_n = (reg_val >> clk.reg_factor_n_offset) & 0x3;

	source = (reg_val >> 24) & 0x3U;
	clk_hz = sunxi_sdhci_clk_source_rate(&clk, source);
	if (clk_hz == 0U) {
		pr_debug("SMHC: unsupported clock source %u\n", source);
		return 0U;
	}

	pr_trace("SMHC: sdhci%d clk parent %uHz, mclk=0x%08x clk_sel=%u, n=%u, m=%u\n", sdhci->id, clk_hz, readl(sdhci->sdhci_clk.reg_base), source, clk.factor_n + 1,
		     clk.factor_m + 1);

	// Calculate the actual clock frequency based on the divider values
	return clk_hz / (clk.factor_n + 1) / (clk.factor_m + 1);
}
