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
	reg_val = BIT(31) | (clk.clk_sel << 24) | ((clk.factor_n - 1) << clk.reg_factor_n_offset) | 
			((clk.factor_m - 1) << clk.reg_factor_m_offset);

    writel(reg_val, clk.reg_base);
	
	printk_trace("SMHC: sdhci%d clk want %uHz parent %uHz, m1div, mclk=0x%08x clk_sel=%u, div=%u, n=%u, m=%u\n", 
					sdhci->id, clk_hz, sclk_hz, readl(sdhci->sdhci_clk.reg_base), clk.clk_sel, div, 
					clk.factor_n, clk.factor_m);
    return 0;
}