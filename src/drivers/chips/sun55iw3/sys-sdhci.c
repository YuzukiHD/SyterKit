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
	
	sunxi_sdhci_clk_t clk = sdhci->sdhci_clk;

    // Determine the clock source based on the requested frequency
    if (clk_hz <= 4000000) {
        clk.clk_sel = 0;// SCLK = 24000000 OSC CLK
    } else {
        clk.clk_sel = 2;// SCLK = AHB PLL
    }

    // Set the clock divider values based on the requested frequency
    switch (clk_hz) {
        case 400000:
            clk.factor_n = 1;
            clk.factor_m = 0xe;
            break;
        case 25000000:
        case 26000000:
            clk.factor_n = (sdhci->id == 2) ? 2 : 1;
            clk.factor_m = (sdhci->id == 2) ? 3 : 2;
            break;
        case 50000000:
        case 52000000:
            clk.factor_n = (sdhci->id == 2) ? 1 : 0;
            clk.factor_m = 2;
            break;
        case 200000000:
            clk.clk_sel = 1;// PERI0_800M
            clk.factor_n = 0;
            clk.factor_m = 1;
            break;
        default:
            clk.factor_n = 0;
            clk.factor_m = 0;
            printk_debug("SMHC: requested frequency does not match: freq=%d\n", clk_hz);
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

/**
 * @brief Set the clock mode for the SDHC controller based on timing mode and other parameters.
 * 
 * This function sets the clock mode for the SDHC controller based on the timing mode and other parameters.
 * 
 * @param sdhci Pointer to the SDHC controller structure.
 * @param clk Clock frequency.
 * @return 0 on success, -1 on failure.
 */
int sunxi_sdhci_clock_mode(sunxi_sdhci_t *sdhci, uint32_t clk) {
    int ret = 0;
    uint32_t reg_val = 0x0;
    sunxi_sdhci_host_t *mmc_host = sdhci->mmc_host;
    sunxi_sdhci_timing_t *timing_data = sdhci->timing_data;
    mmc_t *mmc = sdhci->mmc;
	
	uint32_t module_clk = 0x0;

    /* Reset mclk reg to default */
    writel(0x0, sdhci->sdhci_clk.reg_base);
	
	/* Setting mclk timing */
    if (mmc_host->timing_mode == SUNXI_MMC_TIMING_MODE_1) {
        mmc_host->reg->ntsr |= BIT(31);
        printk_trace("SMHC: rntsr 0x%x\n", mmc_host->reg->ntsr);
    } else {
        mmc_host->reg->ntsr = 0x0;
    }

    if ((mmc_host->timing_mode == SUNXI_MMC_TIMING_MODE_1) || (mmc_host->timing_mode == SUNXI_MMC_TIMING_MODE_3)) {
        /* If using DDR mod to 4 */
        if (mmc->speed_mode == MMC_HSDDR52_DDR50) {
            module_clk = clk * 4;
        } else {
            module_clk = clk * 2;
        }
    } else if (mmc_host->timing_mode == SUNXI_MMC_TIMING_MODE_4) {
        /* If using DDR mod to 4 */
        if ((mmc->speed_mode == MMC_HSDDR52_DDR50) && (mmc->bus_width == SMHC_WIDTH_8BIT)) {
            module_clk = clk * 4;
        } else {
            module_clk = clk * 2;
        }
    }

    /* Now set mclk */
    sunxi_sdhci_set_mclk(sdhci, clk);

    /* Now set clock by mclk we get */
    if ((mmc_host->timing_mode == SUNXI_MMC_TIMING_MODE_1) || (mmc_host->timing_mode == SUNXI_MMC_TIMING_MODE_3)) {
        if (mmc->speed_mode == MMC_HSDDR52_DDR50) {
            mmc->clock = sunxi_sdhci_get_mclk(sdhci) / 4;
        } else {
            mmc->clock = sunxi_sdhci_get_mclk(sdhci) / 2;
        }
    } else if (mmc_host->timing_mode == SUNXI_MMC_TIMING_MODE_4) {
        /* If using DDR mod to 4 */
        if ((mmc->speed_mode == MMC_HSDDR52_DDR50) && (mmc->bus_width == SMHC_WIDTH_8BIT)) {
            mmc->clock = sunxi_sdhci_get_mclk(sdhci) / 4;
        } else {
            mmc->clock = sunxi_sdhci_get_mclk(sdhci) / 2;
        }
    }
    /* Set host clock */
    printk_trace("SMHC: get round clk %luHz, mod_clk %luHz\n", mmc->clock, module_clk);

    /* enable mclk */
    setbits_le32(sdhci->sdhci_clk.reg_base, BIT(31));
    printk_trace("SMHC: mclkbase 0x%x\n", readl(sdhci->sdhci_clk.reg_base));

    /* Configure SMHC_CLKDIV to open clock for devices */
    reg_val = mmc_host->reg->clkcr;
    reg_val &= ~(0xff);
    if ((mmc_host->timing_mode == SUNXI_MMC_TIMING_MODE_1) || (mmc_host->timing_mode == SUNXI_MMC_TIMING_MODE_3)) {
        if (mmc->speed_mode == MMC_HSDDR52_DDR50) {
            reg_val |= 0x1;
        }
    } else if (mmc_host->timing_mode == SUNXI_MMC_TIMING_MODE_4) {
        /* If using DDR mod to 4 */
        if ((mmc->speed_mode == MMC_HSDDR52_DDR50) && (mmc->bus_width == SMHC_WIDTH_8BIT)) {
            reg_val |= 0x1;
        }
    }
    mmc_host->reg->clkcr = reg_val;

    if (sunxi_sdhci_update_clk(sdhci)) {
        return -1;
    }

    /* config delay for mmc device */
    uint32_t freq_id = MMC_CLK_25M;
    switch (clk) {
        case 0 ... 400000:
            freq_id = MMC_CLK_400K;
            break;
        case 400001 ... 26000000:
            freq_id = MMC_CLK_25M;
            break;
        case 26000001 ... 52000000:
            freq_id = MMC_CLK_50M;
            break;
        case 52000001 ... 100000000:
            freq_id = MMC_CLK_100M;
            break;
        case 100000001 ... 150000000:
            freq_id = MMC_CLK_150M;
            break;
        case 150000001 ... 200000000:
            freq_id = MMC_CLK_200M;
            break;
        default:
            freq_id = MMC_CLK_25M;
            break;
    }

    sunxi_sdhci_config_delay(sdhci, mmc->speed_mode, freq_id);

    return 0;
}