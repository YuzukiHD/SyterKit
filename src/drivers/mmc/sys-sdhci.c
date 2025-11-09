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
#include <cache.h>

#include <sys-clk.h>
#include <sys-gpio.h>

#include <mmc/sys-mmc.h>
#include <mmc/sys-sdhci.h>

/* Global data*/
sunxi_sdhci_host_t g_mmc_host;
sunxi_sdhci_timing_t g_mmc_timing;
mmc_t g_mmc;

static void sunxi_sdhci_sync_all_cache(void) {
	flush_dcache_all();
	invalidate_dcache_all();
	data_sync_barrier();
	wmb();
}

/**
 * @brief Enable clock for the SDHC controller.
 * 
 * This function enables clock for the specified SDHC controller.
 * 
 * @param sdhci Pointer to the SDHC controller structure.
 * @return Returns 0 on success.
 */
static int sunxi_sdhci_clk_enable(sunxi_sdhci_t *sdhci) {
	uint32_t reg_val;

	/* Assert AHB Gate and RST clock */
	clrbits_le32(sdhci->clk_ctrl.rst_reg_base, BIT(sdhci->clk_ctrl.rst_reg_offset));
	clrbits_le32(sdhci->clk_ctrl.gate_reg_base, BIT(sdhci->clk_ctrl.gate_reg_offset));

	udelay(10);

	/* Configure AHB Gate and RST clock */
	setbits_le32(sdhci->clk_ctrl.gate_reg_base, BIT(sdhci->clk_ctrl.gate_reg_offset));
	setbits_le32(sdhci->clk_ctrl.rst_reg_base, BIT(sdhci->clk_ctrl.rst_reg_offset));

	/* Enable module clock */
	setbits_le32(sdhci->sdhci_clk.reg_base, BIT(31));

	printk_trace("SMHC: sdhci%d clk enable, mclk: 0x%08x\n", sdhci->id, readl(sdhci->sdhci_clk.reg_base));

	return 0;
}

/**
 * @brief Update clock for the SDHC controller.
 * 
 * This function updates the clock for the specified SDHC controller.
 * 
 * @param sdhci Pointer to the SDHC controller structure.
 * @return Returns 0 on success, -1 on failure.
 */
static int sunxi_sdhci_update_clk(sunxi_sdhci_t *sdhci) {
	sunxi_sdhci_host_t *mmc_host = sdhci->mmc_host;
	uint32_t timeout = time_us() + SMHC_TIMEOUT;

	mmc_host->reg->clkcr |= SMHC_CLKCR_MASK_D0;
	mmc_host->reg->cmd = SMHC_CMD_START | SMHC_CMD_UPCLK_ONLY | SMHC_CMD_WAIT_PRE_OVER;

	/* Wait for process done */
	while ((mmc_host->reg->cmd & SMHC_CMD_START) && (time_us() < timeout)) {}

	/* Check status */
	if (mmc_host->reg->cmd & SMHC_CMD_START) {
		printk_debug("SMHC: mmc %d update clk failed\n", sdhci->id);
		return -1;
	}

	mmc_host->reg->clkcr &= ~SMHC_CLKCR_MASK_D0;
	mmc_host->reg->rint = mmc_host->reg->rint;
	return 0;
}

/**
 * @brief Calculate the timing configuration for Sunxi SD Host Controller using timing mode 4.
 *
 * This function calculates the timing configuration for the Sunxi SD Host Controller when operating
 * in timing mode 4 based on the speed mode and frequency. It updates the timing data with the calculated delay.
 *
 * @param sdhci A pointer to the Sunxi SD Host Controller structure.
 * @param spd_md_id The speed mode ID.
 * @param freq_id The frequency ID.
 *
 * @return 0 on success, -1 on failure.
 */
static int sunxi_sdhci_get_timing_config_timing_4(sunxi_sdhci_t *sdhci, const uint32_t spd_md_id, const uint32_t freq_id) {
	sunxi_sdhci_timing_t *timing_data = sdhci->timing_data;
	uint32_t spd_md_sdly = 0, dly = 0;
	int ret = 0;

	printk_trace("SMHC: sdhci timing config timing 4\n");

	/* Check if the controller ID is MMC_CONTROLLER_2 and if timing mode and frequency ID are valid */
	if ((sdhci->id != MMC_CONTROLLER_2) || (spd_md_id > MMC_HS400) || (freq_id > MMC_MAX_SPD_MD_NUM)) {
		printk_debug("SMHC: timing 4 not supported for this configuration\n");
		return -1;
	}

	/* Calculate delay based on speed mode and frequency */
	spd_md_sdly = sdhci->mmc->tune_sdly.tm4_smx_fx[spd_md_id * 2 + freq_id / 4];
	dly = ((spd_md_sdly >> ((freq_id % 4) * 8)) & 0xff);

	if ((dly == 0xff) || (dly == 0)) {
		if (spd_md_id == MMC_DS26_SDR12) {
			if (freq_id <= MMC_CLK_25M) {
				dly = 0;
			} else {
				printk_debug("SMHC: wrong frequency %d at speed mode %d\n", freq_id, spd_md_id);
				ret = -1;
			}
		} else if (spd_md_id == MMC_HSSDR52_SDR25) {
			if (freq_id <= MMC_CLK_25M) {
				dly = 0;
			} else if (freq_id == MMC_CLK_50M) {
				dly = 15;
			} else {
				printk_debug("SMHC: wrong frequency %d at speed mode %d\n", freq_id, spd_md_id);
				ret = -1;
			}
		} else if (spd_md_id == MMC_HSDDR52_DDR50) {
			if (freq_id <= MMC_CLK_25M) {
				dly = 0;
			} else {
				printk_debug("SMHC: wrong frequency %d at speed mode %d\n", freq_id, spd_md_id);
				ret = -1;
			}
		} else {
			printk_debug("SMHC: wrong speed mode %d\n", spd_md_id);
			ret = -1;
		}
	}

	/* Set output delay based on speed mode */
	if (spd_md_id == MMC_HSDDR52_DDR50) {
		timing_data->odly = 1;
	} else {
		timing_data->odly = 0;
	}

	/* Set the calculated delay */
	timing_data->sdly = dly;

	printk_trace("SMHC: TM4 Timing odly = %u, sdly = %u, spd_md_id = %u, freq_id = %u\n", timing_data->odly, timing_data->sdly, spd_md_id, freq_id);

	return ret;
}

/**
 * @brief Get the timing configuration for the Sunxi SD Host Controller
 *
 * This function retrieves the timing configuration for the Sunxi SD Host Controller based on the provided parameters.
 *
 * @param sdhci     Pointer to the Sunxi SD Host Controller instance
 * @param spd_md_id Speed mode ID
 * @param freq_id   Frequency ID
 *
 * @return          Returns 0 on success, or an error code if the operation fails
 */
static int sunxi_sdhci_get_timing_config(sunxi_sdhci_t *sdhci, uint32_t spd_md_id, uint32_t freq_id) {
	int ret = 0;

	sunxi_sdhci_host_t *mmc_host = sdhci->mmc_host;
	sunxi_sdhci_timing_t *timing_data = sdhci->timing_data;

	// Check for specific conditions based on the controller ID and timing mode
	if ((sdhci->id == 2) && mmc_host->timing_mode == SUNXI_MMC_TIMING_MODE_4) {
		/* When using eMMC and SMHC2, configure it as timing 4 */
		ret = sunxi_sdhci_get_timing_config_timing_4(sdhci, spd_md_id, freq_id);
		if (ret) {
			printk_debug("SMHC: Configuring timing TM4 failed\n");
		}
	} else if ((sdhci->id == 0) && (mmc_host->timing_mode == SUNXI_MMC_TIMING_MODE_1)) {
		// Check timing data and adjust configuration if necessary
		if ((spd_md_id <= MMC_HSSDR52_SDR25) && (freq_id <= MMC_CLK_50M)) {
			/* If timing is less than SDR25, use default odly */
			timing_data->odly = 0;
			timing_data->sdly = 0;
			ret = 0;
		} else {
			printk_warning("SMHC: SMHC0 does not support input speed mode %d\n", spd_md_id);
			ret = -1;
		}
	} else {
		printk_debug("SMHC: Timing setting failed due to parameter error\n");
		ret = -1;
	}

	return ret;
}

/**
 * @brief Configure delay for the Sunxi SD Host Controller
 *
 * This function configures the delay settings for the Sunxi SD Host Controller based on the provided parameters.
 *
 * @param sdhci     Pointer to the Sunxi SD Host Controller instance
 * @param spd_md_id Speed mode ID
 * @param freq_id   Frequency ID
 *
 * @return          Returns 0 on success, or an error code if the operation fails
 */
static int sunxi_sdhci_config_delay(sunxi_sdhci_t *sdhci, uint32_t spd_md_id, uint32_t freq_id) {
	int ret = 0;
	uint32_t reg_val = 0x0;
	sunxi_sdhci_host_t *mmc_host = sdhci->mmc_host;
	mmc_t *mmc = sdhci->mmc;
	sunxi_sdhci_clk_t clk = sdhci->sdhci_clk;
	sunxi_sdhci_timing_t *timing_data = sdhci->timing_data;

	if (mmc_host->timing_mode == SUNXI_MMC_TIMING_MODE_1) {
		timing_data->odly = 0;
		timing_data->sdly = 0;
		printk_trace("SMHC: SUNXI_MMC_TIMING_MODE_1, odly %d, sldy %d\n", timing_data->odly, timing_data->sdly);

		reg_val = mmc_host->reg->drv_dl;
		reg_val &= (~(0x3 << 16));
		reg_val |= (((timing_data->odly & 0x1) << 16) | ((timing_data->odly & 0x1) << 17));
		writel(readl(clk.reg_base) & (~(1 << 31)), clk.reg_base);
		mmc_host->reg->drv_dl = reg_val;
		writel(readl(clk.reg_base) | (1 << 31), clk.reg_base);

		reg_val = mmc_host->reg->ntsr;
		reg_val &= (~(0x3 << 4));
		reg_val |= ((timing_data->sdly & 0x3) << 4);
		mmc_host->reg->ntsr = reg_val;
	} else if (mmc_host->timing_mode == SUNXI_MMC_TIMING_MODE_3) {
		timing_data->odly = 0;
		timing_data->sdly = 0;
		printk_trace("SMHC: SUNXI_MMC_TIMING_MODE_3, odly %d, sldy %d\n", timing_data->odly, timing_data->sdly);

		reg_val = mmc_host->reg->drv_dl;
		reg_val &= (~(0x3 << 16));
		reg_val |= (((timing_data->odly & 0x1) << 16) | ((timing_data->odly & 0x1) << 17));
		writel(readl(clk.reg_base) & (~(1 << 31)), clk.reg_base);
		mmc_host->reg->drv_dl = reg_val;
		writel(readl(clk.reg_base) | (1 << 31), clk.reg_base);

		reg_val = mmc_host->reg->samp_dl;
		reg_val &= (~SDXC_NTDC_CFG_DLY);
		reg_val |= ((timing_data->sdly & SDXC_NTDC_CFG_DLY) | SDXC_NTDC_ENABLE_DLY);
		mmc_host->reg->samp_dl = reg_val;
	} else if (mmc_host->timing_mode == SUNXI_MMC_TIMING_MODE_4) {
		uint32_t spd_md_orig = spd_md_id;

		printk_trace("SMHC: SUNXI_MMC_TIMING_MODE_4, setup freq id: %d, spd_md_id: %d\n", freq_id, spd_md_id);

		if (spd_md_id == MMC_HS400)
			spd_md_id = MMC_HS200_SDR104;

		timing_data->odly = 0xff;
		timing_data->sdly = 0xff;

		if ((ret = sunxi_sdhci_get_timing_config(sdhci, spd_md_id, freq_id)) != 0) {
			printk_debug("SMHC: getting timing param error %d\n", ret);
			return -1;
		}

		if ((timing_data->odly == 0xff) || (timing_data->sdly == 0xff)) {
			printk_debug("SMHC: getting timing config error\n");
			return -1;
		}

		reg_val = mmc_host->reg->drv_dl;
		reg_val &= (~(0x3 << 16));
		reg_val |= (((timing_data->odly & 0x1) << 16) | ((timing_data->odly & 0x1) << 17));
		writel(readl(clk.reg_base) & (~(1 << 31)), clk.reg_base);
		mmc_host->reg->drv_dl = reg_val;
		writel(readl(clk.reg_base) | (1 << 31), clk.reg_base);

		reg_val = mmc_host->reg->samp_dl;
		reg_val &= (~SDXC_NTDC_CFG_DLY);
		reg_val |= ((timing_data->sdly & SDXC_NTDC_CFG_DLY) | SDXC_NTDC_ENABLE_DLY);
		mmc_host->reg->samp_dl = reg_val;

		/* Reset to orig md id */
		spd_md_id = spd_md_orig;
		if (spd_md_id == MMC_HS400) {
			timing_data->odly = 0xff;
			timing_data->sdly = 0xff;
			if ((ret = sunxi_sdhci_get_timing_config(sdhci, spd_md_id, freq_id)) != 0) {
				printk_debug("SMHC: getting timing param error %d\n", ret);
				return -1;
			}

			if ((timing_data->odly == 0xff) || (timing_data->sdly == 0xff)) {
				printk_debug("SMHC: getting timing config error\n");
				return -1;
			}

			reg_val = mmc_host->reg->ds_dl;
			reg_val &= (~SDXC_NTDC_CFG_DLY);
			reg_val |= ((timing_data->sdly & SDXC_NTDC_CFG_DLY) | SDXC_NTDC_ENABLE_DLY);
			mmc_host->reg->ds_dl = reg_val;
		}
		printk_trace("SMHC: config delay freq = %d, odly = %d, sdly = %d, spd_md_id = %d\n", freq_id, timing_data->odly, timing_data->sdly, spd_md_id);
	}
	return ret;
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
	sunxi_sdhci_set_mclk(sdhci, module_clk);

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

/**
 * @brief Configure the clock for the SDHC controller.
 * 
 * This function configures the clock for the SDHC controller based on the specified clock frequency
 * and other parameters.
 * 
 * @param sdhci Pointer to the SDHC controller structure.
 * @param clk Clock frequency.
 * @return 0 on success, -1 on failure.
 */
static int sunxi_sdhci_config_clock(sunxi_sdhci_t *sdhci, uint32_t clk) {
	sunxi_sdhci_host_t *mmc_host = sdhci->mmc_host;
	mmc_t *mmc = sdhci->mmc;

	// Adjust clock frequency if it exceeds the maximum supported frequency for certain speed modes
	if ((mmc->speed_mode == MMC_HSDDR52_DDR50 || mmc->speed_mode == MMC_HS400) && clk > mmc->f_max_ddr) {
		clk = mmc->f_max_ddr;
	}

	// Disable clock before configuration
	mmc_host->reg->clkcr &= ~SMHC_CLKCR_CARD_CLOCK_ON;

	// Update clock settings
	if (sunxi_sdhci_update_clk(sdhci)) {
		printk_debug("SMHC: Failed to update clock settings\n");
		return -1;
	}

	// Configure clock mode based on timing mode
	if (mmc_host->timing_mode == SUNXI_MMC_TIMING_MODE_1 || mmc_host->timing_mode == SUNXI_MMC_TIMING_MODE_3 || mmc_host->timing_mode == SUNXI_MMC_TIMING_MODE_4) {
		if (sunxi_sdhci_clock_mode(sdhci, clk)) {
			printk_debug("SMHC: Failed to configure clock mode\n");
			return -1;
		}
	} else {
		printk_debug("SMHC: Timing mode not supported\n");
		return -1;
	}

	// Enable clock after configuration
	mmc_host->reg->clkcr |= (0x1 << 16);

	// Check if clock update after configuration fails
	if (sunxi_sdhci_update_clk(sdhci)) {
		printk_debug("SMHC: Failed to update clock settings after configuration\n");
		return -1;
	}

	return 0;// Success
}

/**
 * @brief Set DDR mode for the SDHC controller.
 * 
 * This function sets the DDR mode for the SDHC controller based on the specified status.
 * 
 * @param sdhci Pointer to the SDHC controller structure.
 * @param status Boolean indicating whether to enable (true) or disable (false) DDR mode.
 */
static void sunxi_sdhci_ddr_mode_set(sunxi_sdhci_t *sdhci, bool status) {
	uint32_t reg_val = 0x0;
	sunxi_sdhci_host_t *mmc_host = sdhci->mmc_host;

	// Read current value of gctrl register
	reg_val = mmc_host->reg->gctrl;

	// Disable mclk clock
	clrbits_le32(sdhci->sdhci_clk.reg_base, BIT(31));

	// Set or clear DDR mode bit based on status
	if (status) {
		reg_val |= SMHC_GCTRL_DDR_MODE;// Set DDR mode bit
	} else {
		reg_val &= ~SMHC_GCTRL_DDR_MODE;// Clear DDR mode bit
	}

	// Write updated value back to gctrl register
	mmc_host->reg->gctrl = reg_val;

	// Enable mclk clock
	setbits_le32(sdhci->sdhci_clk.reg_base, BIT(31));

	// Log DDR mode status
	printk_trace("SMHC: DDR mode %s\n", status ? "enabled" : "disabled");
}

/**
 * @brief Set HS400 mode for the SDHC controller.
 * 
 * This function sets the HS400 mode for the SDHC controller based on the specified status.
 * 
 * @param sdhci Pointer to the SDHC controller structure.
 * @param status Boolean indicating whether to enable (true) or disable (false) HS400 mode.
 */
static void sunxi_sdhci_hs400_mode_set(sunxi_sdhci_t *sdhci, bool status) {
	uint32_t reg_dsbd_val = 0x0, reg_csdc_val = 0x0;
	sunxi_sdhci_host_t *mmc_host = sdhci->mmc_host;

	// Check if the SDHC controller ID is 2
	if (sdhci->id != 2) {
		// HS400 mode setting is not applicable, return
		return;
	}

	// Read current values of dsbd and csdc registers
	reg_dsbd_val = mmc_host->reg->dsbd;
	reg_csdc_val = mmc_host->reg->csdc;

	// Clear existing bits related to HS400 mode
	reg_dsbd_val &= ~(0x1 << 31);// Clear HS400EN bit
	reg_csdc_val &= ~0xf;		 // Clear HSSDR bit and HS400DS bit

	// Configure HS400 mode based on status
	if (status) {
		// Set HS400EN bit and configure HS400DS to 6 (HS400 mode)
		reg_dsbd_val |= (0x1 << 31);
		reg_csdc_val |= 0x6;
	} else {
		// Configure HS400DS to 3 (backward compatibility mode)
		reg_csdc_val |= 0x3;
	}

	// Write updated values back to dsbd and csdc registers
	mmc_host->reg->dsbd = reg_dsbd_val;
	mmc_host->reg->csdc = reg_csdc_val;

	// Log HS400 mode status
	printk_trace("SMHC: HS400 mode %s\n", status ? "enabled" : "disabled");
}

/**
 * @brief Configure GPIO pins for the SDHC controller.
 * 
 * This function initializes and configures the GPIO pins used by the SDHC controller based on the provided configuration.
 * 
 * @param sdhci Pointer to the SDHC controller structure.
 */
static void sunxi_sdhci_pin_config(sunxi_sdhci_t *sdhci) {
	sunxi_sdhci_pinctrl_t sdhci_pins = sdhci->pinctrl;

	// Initialize and configure GPIO pins for clock, command, and data lines
	sunxi_gpio_init(sdhci_pins.gpio_clk.pin, sdhci_pins.gpio_clk.mux);
	sunxi_gpio_set_pull(sdhci_pins.gpio_clk.pin, GPIO_PULL_UP);

	sunxi_gpio_init(sdhci_pins.gpio_cmd.pin, sdhci_pins.gpio_cmd.mux);
	sunxi_gpio_set_pull(sdhci_pins.gpio_cmd.pin, GPIO_PULL_UP);

	sunxi_gpio_init(sdhci_pins.gpio_d0.pin, sdhci_pins.gpio_d0.mux);
	sunxi_gpio_set_pull(sdhci_pins.gpio_d0.pin, GPIO_PULL_UP);

	// Initialize and configure GPIO pins for 4-bit MMC data bus if applicable
	if (sdhci->width > SMHC_WIDTH_1BIT) {
		sunxi_gpio_init(sdhci_pins.gpio_d1.pin, sdhci_pins.gpio_d1.mux);
		sunxi_gpio_set_pull(sdhci_pins.gpio_d1.pin, GPIO_PULL_UP);

		sunxi_gpio_init(sdhci_pins.gpio_d2.pin, sdhci_pins.gpio_d2.mux);
		sunxi_gpio_set_pull(sdhci_pins.gpio_d2.pin, GPIO_PULL_UP);

		sunxi_gpio_init(sdhci_pins.gpio_d3.pin, sdhci_pins.gpio_d3.mux);
		sunxi_gpio_set_pull(sdhci_pins.gpio_d3.pin, GPIO_PULL_UP);
	}

	// Initialize and configure GPIO pins for 8-bit MMC data bus if applicable
	if (sdhci->width > SMHC_WIDTH_4BIT) {
		sunxi_gpio_init(sdhci_pins.gpio_d4.pin, sdhci_pins.gpio_d4.mux);
		sunxi_gpio_set_pull(sdhci_pins.gpio_d4.pin, GPIO_PULL_UP);

		sunxi_gpio_init(sdhci_pins.gpio_d5.pin, sdhci_pins.gpio_d5.mux);
		sunxi_gpio_set_pull(sdhci_pins.gpio_d5.pin, GPIO_PULL_UP);

		sunxi_gpio_init(sdhci_pins.gpio_d6.pin, sdhci_pins.gpio_d6.mux);
		sunxi_gpio_set_pull(sdhci_pins.gpio_d6.pin, GPIO_PULL_UP);

		sunxi_gpio_init(sdhci_pins.gpio_d7.pin, sdhci_pins.gpio_d7.mux);
		sunxi_gpio_set_pull(sdhci_pins.gpio_d7.pin, GPIO_PULL_UP);

		// Additional GPIO pins for 8-bit MMC configuration
		sunxi_gpio_init(sdhci_pins.gpio_ds.pin, sdhci_pins.gpio_ds.mux);
		sunxi_gpio_set_pull(sdhci_pins.gpio_ds.pin, GPIO_PULL_DOWN);

		sunxi_gpio_init(sdhci_pins.gpio_rst.pin, sdhci_pins.gpio_rst.mux);
		sunxi_gpio_set_pull(sdhci_pins.gpio_rst.pin, GPIO_PULL_UP);
	}

	if (sdhci->pinctrl.gpio_cd.pin != 0) {
		sunxi_gpio_init(sdhci_pins.gpio_cd.pin, sdhci_pins.gpio_cd.mux);
		sunxi_gpio_set_pull(sdhci_pins.gpio_cd.pin, GPIO_PULL_UP);
	}
}

/**
 * @brief Transfer data between SDHC controller and host CPU.
 * 
 * This function handles the data transfer between the SDHC controller and the host CPU.
 * 
 * @param sdhci Pointer to the SDHC controller structure.
 * @param data Pointer to the MMC data structure containing transfer information.
 * @return 0 on success, -1 on failure.
 */
static int sunxi_sunxi_sdhci_trans_data_cpu(sunxi_sdhci_t *sdhci, mmc_data_t *data) {
	sunxi_sdhci_host_t *mmc_host = sdhci->mmc_host;
	uint32_t timeout = time_us() + SMHC_TIMEOUT;
	uint32_t *buff;

	// Determine the buffer based on the direction of data transfer
	if (data->flags & MMC_DATA_READ) {
		buff = (uint32_t *) data->b.dest;// Destination buffer for read operation
		for (size_t i = 0; i < ((data->blocksize * data->blocks) >> 2); i++) {
			while (mmc_host->reg->status & SMHC_STATUS_FIFO_EMPTY && (time_us() < timeout)) {}
			if (mmc_host->reg->status & SMHC_STATUS_FIFO_EMPTY) {
				if (time_us() >= timeout) {
					printk_debug("SMHC: read by CPU failed, timeout, index %u\n", i);
				}
				return -1;
			}
			buff[i] = mmc_host->reg->fifo;
			timeout = time_us() + SMHC_TIMEOUT;// Update timeout for next iteration
		}
	} else {
		buff = (uint32_t *) data->b.src;// Source buffer for write operation
	}

	return 0;// Return success indication
}

/**
 * @brief Transfer data between SDHC controller and host CPU using DMA.
 * 
 * This function handles the data transfer between the SDHC controller and the host CPU
 * using Direct Memory Access (DMA).
 * 
 * @param sdhci Pointer to the SDHC controller structure.
 * @param data Pointer to the MMC data structure containing transfer information.
 * @return void
 */
static int sunxi_sunxi_sdhci_trans_data_dma(sunxi_sdhci_t *sdhci, mmc_data_t *data) {
	sunxi_sdhci_host_t *mmc_host = sdhci->mmc_host;
	sunxi_sdhci_desc_t *pdes = mmc_host->sdhci_desc;
	uint32_t byte_cnt = data->blocksize * data->blocks;
	uint8_t *buff;
	uint32_t des_idx = 0, buff_frag_num = 0, remain = 0;
	uint32_t reg_val = 0;
	uint32_t timeout = time_us() + SMHC_TIMEOUT;

	buff = data->flags & MMC_DATA_READ ? (uint8_t *) data->b.dest : (uint8_t *) data->b.src;
	buff_frag_num = byte_cnt >> SMHC_DES_NUM_SHIFT;
	remain = byte_cnt & (SMHC_DES_BUFFER_MAX_LEN - 1);
	if (remain) {
		buff_frag_num++;
	} else {
		remain = SMHC_DES_BUFFER_MAX_LEN;
	}

	sunxi_sdhci_sync_all_cache();

	for (size_t i = 0; i < buff_frag_num; i++, des_idx++) {
		memset((void *) &pdes[des_idx], 0, sizeof(sunxi_sdhci_desc_t));
		pdes[des_idx].des_chain = 1;
		pdes[des_idx].own = 1;
		pdes[des_idx].dic = 1;

		if (buff_frag_num > 1 && i != buff_frag_num - 1) {
			pdes[des_idx].data_buf_sz = SMHC_DES_BUFFER_MAX_LEN;
		} else {
			pdes[des_idx].data_buf_sz = remain;
		}

		pdes[des_idx].buf_addr = ((size_t) buff + i * SMHC_DES_BUFFER_MAX_LEN) >> 2;
		if (i == 0) {
			pdes[des_idx].first_desc = 1;
		}

		if (i == buff_frag_num - 1) {
			pdes[des_idx].dic = 0;
			pdes[des_idx].last_desc = 1;
			pdes[des_idx].end_of_ring = 1;
			pdes[des_idx].next_desc_addr = 0;
		} else {
			pdes[des_idx].next_desc_addr = ((size_t) &pdes[des_idx + 1]) >> 2;
		}

#ifndef SMHC_DMA_TRACE
		printk_trace("SMHC: frag %d, remain %d, des[%d] = 0x%08x:"
					 "  [0] = 0x%08x, [1] = 0x%08x, [2] = 0x%08x, [3] = 0x%08x\n",
					 i, remain, des_idx, (uint32_t) (&pdes[des_idx]), (uint32_t) ((uint32_t *) &pdes[des_idx])[0], (uint32_t) ((uint32_t *) &pdes[des_idx])[1],
					 (uint32_t) ((uint32_t *) &pdes[des_idx])[2], (uint32_t) ((uint32_t *) &pdes[des_idx])[3]);
#endif// SMHC_DMA_TRACE
	}
	sunxi_sdhci_sync_all_cache();
	/*
	 * GCTRLREG
	 * GCTRL[2]     : DMA reset
	 * GCTRL[5]     : DMA enable
	 *
	 * IDMACREG
	 * IDMAC[0]     : IDMA soft reset
	 * IDMAC[1]     : IDMA fix burst flag
	 * IDMAC[7]     : IDMA on
	 *
	 * IDIECREG
	 * IDIE[0]      : IDMA transmit interrupt flag
	 * IDIE[1]      : IDMA receive interrupt flag
	 */


	/* Enable DMA */
	// mmc_host->reg->idst = 0x337;
	mmc_host->reg->gctrl |= (SMHC_GCTRL_DMA_ENABLE | SMHC_GCTRL_DMA_RESET);

	timeout = time_us() + SMHC_TIMEOUT;
	while (mmc_host->reg->gctrl & SMHC_GCTRL_DMA_RESET) {
		if (time_us() > timeout) {
			printk_debug("SMHC: wait for dma rst timeout\n");
			return -1;
		}
	}

	/* DMA Reset */
	mmc_host->reg->dmac = SMHC_IDMAC_SOFT_RESET;
	timeout = time_us() + SMHC_TIMEOUT;
	while (mmc_host->reg->dmac & SMHC_IDMAC_SOFT_RESET) {
		if (time_us() > timeout) {
			printk_debug("SMHC: wait for dma soft rst timeout\n");
			return -1;
		}
	}

	/* DMA on */
	mmc_host->reg->dmac = (SMHC_IDMAC_FIX_BURST | SMHC_IDMAC_IDMA_ON);

	/* Set DMA INTERRUPT */
	mmc_host->reg->idie &= ~(SMHC_IDMAC_TRANSMIT_INTERRUPT | SMHC_IDMAC_RECEIVE_INTERRUPT);
	if (data->flags & MMC_DATA_WRITE) {
		mmc_host->reg->idie |= SMHC_IDMAC_TRANSMIT_INTERRUPT;
	} else {
		mmc_host->reg->idie |= SMHC_IDMAC_RECEIVE_INTERRUPT;
	}

	mmc_host->reg->dlba = (uint32_t) pdes >> 2;

	/* burst-16, rx/tx trigger level=15/240 */
	mmc_host->reg->ftrglevel = ((3 << 28) | (15 << 16) | 240);

	return 0;
}

/**
 * @brief Set the I/O settings for the SDHC controller.
 * 
 * This function configures the I/O settings for the SDHC controller based on the
 * provided MMC clock, bus width, and speed mode.
 * 
 * @param sdhci Pointer to the SDHC controller structure.
 * @return void
 */
void sunxi_sdhci_set_ios(sunxi_sdhci_t *sdhci) {
	sunxi_sdhci_host_t *mmc_host = sdhci->mmc_host;
	mmc_t *mmc = sdhci->mmc;

	printk_trace("SMHC: ios setting bus:%u, speed %u\n", mmc->bus_width, mmc->clock);

	// Configure clock and handle errors
	if (mmc->clock && sunxi_sdhci_config_clock(sdhci, mmc->clock)) {
		printk_debug("SMHC: update clock failed\n");
		mmc_host->fatal_err = 1;
		return;
	}

	/* Change bus width */
	mmc_host->reg->width = mmc->bus_width;

	/* Set DDR mode */
	if (mmc->speed_mode == MMC_HSDDR52_DDR50) {
		sunxi_sdhci_ddr_mode_set(sdhci, true);
		sunxi_sdhci_hs400_mode_set(sdhci, false);
	} else if (mmc->speed_mode == MMC_HS400) {
		sunxi_sdhci_ddr_mode_set(sdhci, false);
		sunxi_sdhci_hs400_mode_set(sdhci, true);
	} else {
		sunxi_sdhci_ddr_mode_set(sdhci, false);
		sunxi_sdhci_hs400_mode_set(sdhci, false);
	}
}

/**
 * @brief Initialize the core functionality of the SDHC controller.
 * 
 * This function initializes the core functionality of the SDHC controller,
 * including resetting the controller, setting timeout values, configuring
 * thresholds and debug parameters, and releasing the eMMC reset signal.
 * 
 * @param sdhci Pointer to the SDHC controller structure.
 * @return Returns 0 on success, -1 on failure.
 */
int sunxi_sdhci_core_init(sunxi_sdhci_t *sdhci) {
	uint32_t reg_val = 0x0;
	uint32_t timeout = time_us() + SMHC_TIMEOUT;
	sunxi_sdhci_host_t *mmc_host = sdhci->mmc_host;
	mmc_t *mmc = sdhci->mmc;

	/* Reset controller */
	mmc_host->reg->gctrl = 0x7;
	while (mmc_host->reg->gctrl & 0x7) {
		if (time_us() > timeout) {
			printk_debug("SMHC: controller reset timeout\n");
			return -1;
		}
	}

	/* Set Data & Response Timeout Value */
	mmc_host->reg->timeout = (SMHC_DATA_TIMEOUT << 8) | SMHC_RESP_TIMEOUT;

	mmc_host->reg->thldc = ((512 << 16) | (0x1 << 2) | (0x1 << 0));
	mmc_host->reg->csdc = 0x3;
	mmc_host->reg->dbgc = 0xdeb;

	/* Release eMMC RST */
	mmc_host->reg->hwrst = 1;
	mmc_host->reg->hwrst = 0;
	mdelay(1);
	mmc_host->reg->hwrst = 1;
	mdelay(1);

	if (sdhci->id == 0) {
		/* Enable 2xclk mode, and use default input phase */
		mmc_host->reg->ntsr |= (0x1 << 31);
	} else {
		/* Configure input delay time to 0, use default input phase */
		reg_val = mmc_host->reg->samp_dl;
		reg_val &= ~(0x3f);
		reg_val |= (0x1 << 7);
		mmc_host->reg->samp_dl = reg_val;
	}

	return 0;
}

/**
 * @brief Perform a data transfer operation on the SDHC controller.
 * 
 * This function performs a data transfer operation on the SDHC controller,
 * including sending a command and managing data transfer if present. It also
 * handles error conditions such as fatal errors and card busy status.
 * 
 * @param sdhci Pointer to the SDHC controller structure.
 * @param cmd Pointer to the MMC command structure.
 * @param data Pointer to the MMC data structure.
 * @return Returns 0 on success, -1 on failure.
 */
int sunxi_sdhci_xfer(sunxi_sdhci_t *sdhci, mmc_cmd_t *cmd, mmc_data_t *data) {
	sunxi_sdhci_host_t *mmc_host = sdhci->mmc_host;
	uint32_t cmdval = SMHC_CMD_START;
	uint32_t timeout = time_us() + SMHC_TIMEOUT;
	uint32_t status;
	uint32_t reg_val;
	int ret = 0, error_code = 0;
	uint8_t use_dma_status = false;

	/* Check if have fatal error */
	if (mmc_host->fatal_err) {
		printk_debug("SMHC: SMHC into error, cmd send failed\n");
		return -1;
	}

	/* check card busy*/
	if (cmd->resp_type & MMC_RSP_BUSY) {
		printk_trace("SMHC: cmd %u check Card busy\n", cmd->cmdidx);
	}

	/* Check if stop or manual */
	if ((cmd->cmdidx == MMC_CMD_STOP_TRANSMISSION) && !(cmd->flags & MMC_CMD_MANUAL)) {
		return 0;
	}

	/*
	 * CMDREG
	 * CMD[5:0]     : Command index
	 * CMD[6]       : Has response
	 * CMD[7]       : Long response
	 * CMD[8]       : Check response CRC
	 * CMD[9]       : Has data
	 * CMD[10]      : Write
	 * CMD[11]      : Steam mode
	 * CMD[12]      : Auto stop
	 * CMD[13]      : Wait previous over
	 * CMD[14]      : About cmd
	 * CMD[15]      : Send initialization
	 * CMD[21]      : Update clock
	 * CMD[31]      : Load cmd
	 */

	if (!cmd->cmdidx)
		cmdval |= SMHC_CMD_SEND_INIT_SEQUENCE;
	if (cmd->resp_type & MMC_RSP_PRESENT)
		cmdval |= SMHC_CMD_RESP_EXPIRE;
	if (cmd->resp_type & MMC_RSP_136)
		cmdval |= SMHC_CMD_LONG_RESPONSE;
	if (cmd->resp_type & MMC_RSP_CRC)
		cmdval |= SMHC_CMD_CHECK_RESPONSE_CRC;

	if (data) {
		/* Check data desc align */
		if ((uint32_t) data->b.dest & 0x3) {
			printk_debug("SMHC: data dest is not 4 byte align\n");
			error_code = -1;
			goto out;
		}

		cmdval |= SMHC_CMD_DATA_EXPIRE | SMHC_CMD_WAIT_PRE_OVER;
		if (data->flags & MMC_DATA_WRITE) {
			cmdval |= SMHC_CMD_WRITE;
		}
		if (data->blocks > 1) {
			cmdval |= SMHC_CMD_SEND_AUTO_STOP;
		}
		mmc_host->reg->blksz = data->blocksize;
		mmc_host->reg->bytecnt = data->blocks * data->blocksize;
	} else {
		if ((cmd->cmdidx == MMC_CMD_STOP_TRANSMISSION) && (cmd->flags & MMC_CMD_MANUAL)) {
			cmdval |= SMHC_CMD_STOP_ABORT_CMD;//stop current data transferin progress.
			cmdval &= ~SMHC_CMD_WAIT_PRE_OVER;//Send command at once, even if previous data transfer has notcompleted
		}
	}

	printk_trace("SMHC: CMD: %u(0x%08x), arg: 0x%x, dlen:%u\n", cmd->cmdidx, cmdval | cmd->cmdidx, cmd->cmdarg, data ? data->blocks * data->blocksize : 0);

	mmc_host->reg->arg = cmd->cmdarg;

	if (!data) {
		mmc_host->reg->cmd = (cmdval | cmd->cmdidx);
	}

	/*
	 * transfer data and check status
	 * STATREG[2] : FIFO empty
	 * STATREG[3] : FIFO full
	 */

	data_sync_barrier();

	if (data) {
		printk_trace("SMHC: transfer data %lu bytes by %s\n", data->blocksize * data->blocks, (((data->blocksize * data->blocks > 512) && (mmc_host->sdhci_desc)) ? "DMA" : "CPU"));
		if ((data->blocksize * data->blocks > 512) && (mmc_host->sdhci_desc)) {
			use_dma_status = true;
			mmc_host->reg->gctrl &= ~SMHC_GCTRL_ACCESS_BY_AHB;
			ret = sunxi_sunxi_sdhci_trans_data_dma(sdhci, data);
			mmc_host->reg->cmd = (cmdval | cmd->cmdidx);
		} else {
			mmc_host->reg->gctrl |= SMHC_GCTRL_ACCESS_BY_AHB;
			mmc_host->reg->cmd = (cmdval | cmd->cmdidx);
			ret = sunxi_sunxi_sdhci_trans_data_cpu(sdhci, data);
		}

		if (ret) {
			error_code = mmc_host->reg->rint & SMHC_RINT_INTERRUPT_ERROR_BIT;
			printk_debug("SMHC: error 0x%x status 0x%x\n", error_code & SMHC_RINT_INTERRUPT_ERROR_BIT, error_code & ~SMHC_RINT_INTERRUPT_ERROR_BIT);
			if (!error_code) {
				error_code = 0xffffffff;
			}
			goto out;
		}
	}

	timeout = time_us() + SMHC_TIMEOUT;
	do {
		status = mmc_host->reg->rint;
		if ((time_us() > timeout) || (status & SMHC_RINT_INTERRUPT_ERROR_BIT)) {
			error_code = status & SMHC_RINT_INTERRUPT_ERROR_BIT;
			if (!error_code) {
				error_code = 0xffffffff;
			}
			if (time_us() > timeout)
				printk_debug("SMHC: stage 1 data timeout, error %08x\n", error_code);
			else
				printk_debug("SMHC: stage 1 status get interrupt, error 0x%08x\n", error_code);
			goto out;
		}
	} while (!(status & SMHC_RINT_COMMAND_DONE));

	if (data) {
		uint32_t done = false;
		timeout = time_us() + (use_dma_status ? SMHC_DMA_TIMEOUT : SMHC_TIMEOUT);
		do {
			status = mmc_host->reg->rint;
			if ((time_us() > timeout) || (status & SMHC_RINT_INTERRUPT_ERROR_BIT)) {
				error_code = status & SMHC_RINT_INTERRUPT_ERROR_BIT;
				if (!error_code) {
					error_code = 0xffffffff;
				}
				if (time_us() > timeout)
					printk_debug("SMHC: stage 2 data timeout, error %08x\n", error_code);
				else
					printk_debug("SMHC: stage 2 status get interrupt, error 0x%08x\n", error_code);
				goto out;
			}

			if (data->blocks > 1) {
				done = status & SMHC_RINT_AUTO_COMMAND_DONE;
			} else {
				done = status & SMHC_RINT_DATA_OVER;
			}
		} while (!done);

		if ((data->flags & MMC_DATA_READ) && use_dma_status) {
			timeout = time_us() + SMHC_DMA_TIMEOUT;
			done = 0;
			status = 0;
			do {
				status = mmc_host->reg->idst;
				if ((time_us() > timeout) || (status & 0x234)) {
					error_code = status & 0x1e34;
					if (!error_code) {
						error_code = 0xffffffff;
					}
					printk_debug("SMHC: wait dma timeout, error %08x\n", error_code);
					goto out;
				}
				done = status & BIT(1);
			} while (!done);
		}
	}

	if (cmd->resp_type & MMC_RSP_BUSY) {
		timeout = time_us() + SMHC_WAITBUSY_TIMEOUT;
		do {
			status = mmc_host->reg->status;
			if ((time_us() > timeout)) {
				error_code = -1;
				if (!error_code) {
					error_code = 0xffffffff;
				}
				printk_debug("SMHC: busy timeout, status %08x\n", status);
				goto out;
			}
		} while (status & SMHC_STATUS_CARD_DATA_BUSY);
	}

	if (cmd->resp_type & MMC_RSP_136) {
		cmd->response[0] = mmc_host->reg->resp3;
		cmd->response[1] = mmc_host->reg->resp2;
		cmd->response[2] = mmc_host->reg->resp1;
		cmd->response[3] = mmc_host->reg->resp0;
		printk_trace("SMHC: resp 0x%08x 0x%08x 0x%08x 0x%08x\n", cmd->response[3], cmd->response[2], cmd->response[1], cmd->response[0]);
	} else {
		cmd->response[0] = mmc_host->reg->resp0;
		printk_trace("SMHC: resp 0x%08x\n", cmd->response[0]);
	}

out:
	if (data && use_dma_status) {
		/* IDMASTAREG Reset controller status
		 * IDST[0] : idma tx int
		 * IDST[1] : idma rx int
		 * IDST[2] : idma fatal bus error
		 * IDST[4] : idma descriptor invalid
		 * IDST[5] : idma error summary
		 * IDST[8] : idma normal interrupt sumary
		 * IDST[9] : idma abnormal interrupt sumary
		 */
		status = mmc_host->reg->idst;
		mmc_host->reg->idst = status;
		mmc_host->reg->idie = 0;
		mmc_host->reg->dmac = 0;
		mmc_host->reg->gctrl &= ~SMHC_GCTRL_DMA_ENABLE;
	}

	sunxi_sdhci_sync_all_cache();

	if (error_code) {
		mmc_host->reg->gctrl = SMHC_GCTRL_HARDWARE_RESET;
		timeout = time_us() + SMHC_TIMEOUT;
		while (mmc_host->reg->gctrl & SMHC_GCTRL_HARDWARE_RESET) {
			if (time_us() > timeout) {
				printk_debug("SMHC: controller error reset timeout\n");
				return -1;
			}
		}
		sunxi_sdhci_update_clk(sdhci);
		printk_debug("SMHC: CMD 0x%08x, error 0x%08x\n", cmd->cmdidx, error_code);
	}

	mmc_host->reg->rint = 0xffffffff;

	if (error_code) {
		return -1;
	}

	return 0;
}

/**
 * @brief Update phase for the SDHC controller.
 * 
 * This function updates the phase for the specified SDHC controller.
 * 
 * @param sdhci Pointer to the SDHC controller structure.
 * @return Returns 0 on success.
 */
int sunxi_sdhci_update_phase(sunxi_sdhci_t *sdhci) {
	sunxi_sdhci_host_t *mmc_host = sdhci->mmc_host;

	// Check if timing mode is mode 1
	if (mmc_host->timing_mode == SUNXI_MMC_TIMING_MODE_1) {
		printk_trace("SMHC: mmc update phase.\n");
		// Call function to update clock
		return sunxi_sdhci_update_clk(sdhci);
	}

	return 0;
}

/**
 * @brief Initialize the SDHC controller.
 * 
 * This function initializes the SDHC controller by configuring its parameters,
 * capabilities, and features based on the provided SDHC structure. It sets up
 * the controller's timing mode, supported voltages, host capabilities, clock
 * frequency limits, and register addresses. Additionally, it configures pin
 * settings and enables clocks for the SDHC controller.
 * 
 * @param sdhci Pointer to the SDHC structure.
 * @return Returns 0 on success, -1 on failure.
 */
int sunxi_sdhci_init(sunxi_sdhci_t *sdhci) {
	/* Check if controller ID is correct */
	if (sdhci->id > MMC_CONTROLLER_2) {
		printk_debug("SMHC: Unsupported MAX Controller reached\n");
		return -1;
	}

	/* init resource */
	memset(&g_mmc_host, 0, sizeof(sunxi_sdhci_host_t));
	sdhci->mmc_host = &g_mmc_host;
	sunxi_sdhci_host_t *mmc_host = sdhci->mmc_host;

	memset(&g_mmc, 0, sizeof(mmc_t));
	sdhci->mmc = &g_mmc;
	mmc_t *mmc = sdhci->mmc;

	memset(&g_mmc_timing, 0, sizeof(sunxi_sdhci_timing_t));
	sdhci->timing_data = &g_mmc_timing;

	/* Set timing mode based on controller ID */
	if (sdhci->id == MMC_CONTROLLER_0) {
		mmc_host->timing_mode = SUNXI_MMC_TIMING_MODE_1;
	} else if (sdhci->id == MMC_CONTROLLER_2) {
		mmc_host->timing_mode = SUNXI_MMC_TIMING_MODE_4;
	}

	/* Set supported voltages and host capabilities */
	mmc->voltages = MMC_VDD_29_30 | MMC_VDD_30_31 | MMC_VDD_31_32 | MMC_VDD_32_33 | MMC_VDD_33_34 | MMC_VDD_34_35 | MMC_VDD_35_36;
	mmc->host_caps = MMC_MODE_HS_52MHz | MMC_MODE_HS | MMC_MODE_HC;

	/* Set host capabilities for bus width */
	if (sdhci->width >= SMHC_WIDTH_4BIT) {
		mmc->host_caps |= MMC_MODE_4BIT;
	}
	if ((sdhci->id == MMC_CONTROLLER_2) && (sdhci->width == SMHC_WIDTH_8BIT)) {
		mmc->host_caps |= MMC_MODE_8BIT | MMC_MODE_4BIT;
	}

	/* Set clock frequency limits */
	mmc->f_min = 400000;
	mmc->f_max = sdhci->max_clk;
	mmc->f_max_ddr = sdhci->max_clk;

	/* Set register addresses */
	mmc_host->reg = (sdhci_reg_t *) sdhci->reg_base;
	if (sdhci->dma_des_addr == 0)
		mmc_host->sdhci_desc = NULL;
	else
		mmc_host->sdhci_desc = (sunxi_sdhci_desc_t *) sdhci->dma_des_addr;

	/* Configure pins and enable clocks */
	sunxi_sdhci_pin_config(sdhci);
	sunxi_sdhci_clk_enable(sdhci);

	return 0;
}

/**
 * @brief Dump the contents of the SDHCI registers.
 *
 * This function dumps the contents of the SDHCI registers for a given SD card host controller.
 *
 * @param sdhci A pointer to the structure representing the SD card host controller.
 * @return void
 *
 * @note This function is useful for debugging and analyzing the state of the SD card controller.
 */
void sunxi_sdhci_dump_reg(sunxi_sdhci_t *sdhci) {
	sdhci_reg_t *reg = sdhci->mmc_host->reg;
	printk_trace("=========SDHCI%d REG========\n", sdhci->id);
	printk_trace("gctrl     0x%x\n", reg->gctrl);
	printk_trace("clkcr     0x%x\n", reg->clkcr);
	printk_trace("timeout   0x%x\n", reg->timeout);
	printk_trace("width     0x%x\n", reg->width);
	printk_trace("blksz     0x%x\n", reg->blksz);
	printk_trace("bytecnt   0x%x\n", reg->bytecnt);
	printk_trace("cmd       0x%x\n", reg->cmd);
	printk_trace("arg       0x%x\n", reg->arg);
	printk_trace("resp0     0x%x\n", reg->resp0);
	printk_trace("resp1     0x%x\n", reg->resp1);
	printk_trace("resp2     0x%x\n", reg->resp2);
	printk_trace("resp3     0x%x\n", reg->resp3);
	printk_trace("imask     0x%x\n", reg->imask);
	printk_trace("mint      0x%x\n", reg->mint);
	printk_trace("rint      0x%x\n", reg->rint);
	printk_trace("status    0x%x\n", reg->status);
	printk_trace("ftrglevel 0x%x\n", reg->ftrglevel);
	printk_trace("funcsel   0x%x\n", reg->funcsel);
	printk_trace("dmac      0x%x\n", reg->dmac);
	printk_trace("dlba      0x%x\n", reg->dlba);
	printk_trace("idst      0x%x\n", reg->idst);
	printk_trace("idie      0x%x\n", reg->idie);
	printk_trace("drv_dl    0x%x\n", reg->drv_dl);
	printk_trace("samp_dl   0x%x\n", reg->samp_dl);
	printk_trace("ds_dl     0x%x\n", reg->ds_dl);
	printk_trace("\n\n");
}
