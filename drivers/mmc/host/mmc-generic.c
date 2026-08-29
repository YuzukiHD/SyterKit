/* SPDX-License-Identifier: GPL-2.0+ */
#define pr_fmt(fmt) "mmc-generic: " fmt

#include <stdint.h>

#include <io.h>
#include <log.h>

#include <drivers/mmc/sdhci.h>

/*
 * Default module-clock implementation for the common Allwinner layout.
 * SoCs with a different divider or DDR clock relationship override these
 * weak symbols in their host file.
 */
int __attribute__((weak, section(".text.mmc_generic_set_mclk")))
sunxi_sdhci_set_mclk(sunxi_sdhci_t *sdhci, uint32_t clk_hz)
{
	uint32_t reg_val;
	uint32_t source;
	uint32_t source_rate;
	uint32_t div;
	sunxi_sdhci_clk_t clk;

	if (sdhci == NULL || clk_hz == 0U || sdhci->sdhci_clk.reg_base == 0U)
		return -1;

	clk = sdhci->sdhci_clk;
	source = clk_hz <= 4000000U ? 0U : clk.default_clk_sel;
	source_rate = sunxi_sdhci_clk_source_rate(&clk, source);
	if (source_rate == 0U) {
		pr_debug("unsupported clock source %u\n", source);
		return -1;
	}

	div = (2U * source_rate + clk_hz) / (2U * clk_hz);
	div = div == 0U ? 1U : div;

	if (div > 128U) {
		clk.factor_n = 0U;
		clk.factor_m = 1U;
		pr_warn("source clock is too high\n");
	} else if (div > 64U) {
		clk.factor_n = 3U;
		clk.factor_m = div >> 3;
	} else if (div > 32U) {
		clk.factor_n = 2U;
		clk.factor_m = div >> 2;
	} else if (div > 16U) {
		clk.factor_n = 1U;
		clk.factor_m = div >> 1;
	} else {
		clk.factor_n = 0U;
		clk.factor_m = div;
	}

	reg_val = BIT(31) | (source << 24) |
		((uint32_t)clk.factor_n << clk.reg_factor_n_offset) |
		((uint32_t)(clk.factor_m - 1U) << clk.reg_factor_m_offset);
	writel(reg_val, clk.reg_base);

	pr_trace("sdhci%u clk want %uHz parent %uHz, div=%u, n=%u, m=%u\n",
		 sdhci->id, clk_hz, source_rate, div, clk.factor_n, clk.factor_m);
	return 0;
}

uint32_t __attribute__((weak, section(".text.mmc_generic_get_mclk")))
sunxi_sdhci_get_mclk(sunxi_sdhci_t *sdhci)
{
	uint32_t reg_val;
	uint32_t source;
	uint32_t source_rate;
	sunxi_sdhci_clk_t clk;

	if (sdhci == NULL || sdhci->sdhci_clk.reg_base == 0U)
		return 0U;

	clk = sdhci->sdhci_clk;
	reg_val = readl(clk.reg_base);
	clk.factor_m = (reg_val >> clk.reg_factor_m_offset) & 0xfU;
	clk.factor_n = (reg_val >> clk.reg_factor_n_offset) & 0x3U;
	source = (reg_val >> 24) & 0x3U;

	source_rate = sunxi_sdhci_clk_source_rate(&clk, source);
	if (source_rate == 0U) {
		pr_debug("unsupported clock source %u\n", source);
		return 0U;
	}

	return source_rate / (clk.factor_n + 1U) / (clk.factor_m + 1U);
}
