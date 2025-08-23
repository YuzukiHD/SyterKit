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

#include <mmu.h>

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
	uint32_t reg_val = 0x0, sclk_hz = 0x0, div = 0x0;

	sunxi_sdhci_clk_t clk = sdhci->sdhci_clk;

	if (clk_hz <= 4000000) {
		clk.clk_sel = 0;
		sclk_hz = sunxi_clk_get_hosc_type() * 1000 * 1000; /* use hosc clk */
	} else {
		if (clk.clk_sel && clk.parent_clk) {
			sclk_hz = clk.parent_clk;
		} else {
			clk.clk_sel = 1;
			sclk_hz = sunxi_clk_get_peri1x_rate() * 2 * 1000 * 1000; /* use 2x pll-per0 clock */
		}
	}

	div = sclk_hz / clk_hz;
	if (sclk_hz % clk_hz)
		div += 1;

	for (clk.factor_n = 1; clk.factor_n <= 32; clk.factor_n++) {
		for (clk.factor_m = clk.factor_n; clk.factor_m <= 32; clk.factor_m++) {
			if (clk.factor_n * clk.factor_m == div) {
				goto set_mclk;
			}
		}
	}
	printk_warning("SMHC: Illegal frequency division parameters %d\n", div);

set_mclk:
	reg_val = BIT(31) | (clk.clk_sel << 24) | ((clk.factor_n - 1) << clk.reg_factor_n_offset) | ((clk.factor_m - 1) << clk.reg_factor_m_offset);

	writel(reg_val, clk.reg_base);

	printk_trace("SMHC: sdhci%d clk want %uHz parent %uHz, m1div, mclk=0x%08x clk_sel=%u, div=%u, n=%u, m=%u\n", sdhci->id, clk_hz, sclk_hz, readl(sdhci->sdhci_clk.reg_base),
				 clk.clk_sel, div, clk.factor_n, clk.factor_m);
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
	uint32_t clk_hz = 0x0;
	uint32_t reg_val = 0x0;
	sunxi_sdhci_clk_t clk = sdhci->sdhci_clk;

	// Read the clock register value
	reg_val = readl(clk.reg_base);

	// Extract the divider values and clock source from the register value
	clk.factor_m = (reg_val >> clk.reg_factor_m_offset) & 0xf;
	clk.factor_n = (reg_val >> clk.reg_factor_n_offset) & 0x3;

	// check clk sel, check if correct
	if (clk.clk_sel != ((reg_val >> 24) & 0x3)) {
		clk.clk_sel = ((reg_val >> 24) & 0x3);
	}

	// Calculate the current clock frequency based on the source and divider values
	switch (clk.clk_sel) {
		case 0:
			clk_hz = sunxi_clk_get_hosc_type() * 1000 * 1000;
			break;
		case 1:
			if (clk.parent_clk && clk.clk_sel) {
				clk_hz = clk.parent_clk;
			} else {
				clk_hz = sunxi_clk_get_peri1x_rate() * 2 * 1000 * 1000;
			}
			break;
		default:
			printk_debug("SMHC: wrong clock source %u\n", clk.clk_sel);
			break;
	}

	printk_trace("SMHC: sdhci%d clk parent %uHz, mclk=0x%08x clk_sel=%u, n=%u, m=%u\n", sdhci->id, clk_hz, readl(sdhci->sdhci_clk.reg_base), clk.clk_sel, clk.factor_n + 1,
				 clk.factor_m + 1);

	// Calculate the actual clock frequency based on the divider values
	return clk_hz / (clk.factor_n + 1) / (clk.factor_m + 1);
}