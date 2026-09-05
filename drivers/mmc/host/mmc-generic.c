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
/**
 * @brief Set the SDHC controller clock frequency using the generic divider
 * @details Computes the nearest clock divider between the selected parent
 *          clock and the requested frequency, encodes it as factor n/m
 *          register fields, and writes the clock register.  Selects the
 *          low-frequency source for <= 4 MHz and the default source otherwise.
 * @param sdhci Pointer to the SDHC controller structure
 * @param clk_hz Desired clock frequency in Hertz
 * @return 0 on success, -1 on invalid arguments or an unsupported clock source
 */
int __attribute__((weak, section(".text.mmc_generic_set_mclk"))) sunxi_sdhci_set_mclk(
	sunxi_sdhci_t *sdhci, uint32_t clk_hz)
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

	reg_val = BIT(31) | (source << 24) | ((uint32_t)clk.factor_n << clk.reg_factor_n_offset) |
		  ((uint32_t)(clk.factor_m - 1U) << clk.reg_factor_m_offset);
	writel(reg_val, clk.reg_base);

	pr_trace("sdhci%u clk want %uHz parent %uHz, div=%u, n=%u, m=%u\n", sdhci->id, clk_hz, source_rate, div,
		clk.factor_n, clk.factor_m);
	return 0;
}

/**
 * @brief Read back the current SDHC controller clock frequency
 * @details Reads the clock register and decodes the clock source and factor
 *          n/m fields, then derives the resulting frequency as the parent
 *          rate divided by (n+1) and (m+1).
 * @param sdhci Pointer to the SDHC controller structure
 * @return The current clock frequency in Hertz, or 0 on invalid arguments or
 *         an unsupported clock source
 */
uint32_t __attribute__((weak, section(".text.mmc_generic_get_mclk"))) sunxi_sdhci_get_mclk(sunxi_sdhci_t *sdhci)
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

/*
 * Default I/O-voltage implementation: the common Allwinner MMC host has no
 * programmable I/O rail, so this is a no-op.  SoCs with a power-mode
 * selector (e.g. Sun55 GPIO v2-pow) override this weak symbol in their
 * host file.
 */
/**
 * @brief Set the MMC I/O voltage using the generic no-op implementation
 * @details Default weak implementation for the common Allwinner MMC host,
 *          which has no programmable I/O rail, so it performs no action and
 *          always reports success.  SoCs with a power-mode selector override
 *          this symbol in their host file.
 * @param sdhci Pointer to the SDHC controller structure
 * @param gpio GPIO pin identifying the physical I/O bank
 * @param voltage_uv Requested I/O voltage in microvolts
 * @return Always 0
 */
int __attribute__((weak, section(".text.mmc_generic_set_io_voltage"))) sunxi_sdhci_set_io_voltage(
	sunxi_sdhci_t *sdhci, const gpio_mux_t *gpio, uint32_t voltage_uv)
{
	return 0;
}

/*
 * Default data-line deskew implementation: the common Allwinner MMC host
 * needs no fixed deskew, so this is a no-op.  SoCs that require one on some
 * controllers (e.g. MMC2 on Sun55iw6) override this weak symbol in their
 * host file.
 */
/**
 * @brief Set the MMC data-line deskew using the generic no-op implementation
 * @details Default weak implementation that performs no deskew programming
 *          and always reports success.  SoCs that require a fixed data-line
 *          deskew on some controllers override this symbol in their host file.
 * @param sdhci Pointer to the SDHC controller structure
 * @return Always 0
 */
int __attribute__((weak, section(".text.mmc_generic_set_skew"))) sunxi_sdhci_set_skew(
	sunxi_sdhci_t *sdhci)
{
	return 0;
}
