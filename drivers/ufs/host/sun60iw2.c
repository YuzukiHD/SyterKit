/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file sun60iw2.c
 * @brief sun60iw2 UFS host variant.
 *
 * Provides the sun60iw2 UFS PHY and clock variant configuration and reads the
 * PHY calibration words from the SID eFuse SRAM.
 */

#include <drivers/clk/sun60iw2/reg.h>
#include <log.h>
#include <drivers/sid/sid.h>
#include <drivers/ufs/host/sunxi.h>
#include <dt-compatible/sid-dt.h>

/** @brief sun60iw2 UFS PHY and clock variant configuration. */
static const struct sunxi_ufs_variant sun60iw2_ufs_variant = {
	.cal_low_offset = 0x60U,
	.cal_high_offset = 0x64U,
	.ext_res_ctrl = SUNXI_SYSCTRL_BASE + 0x160U,
	.ext_res1_ctrl = SUNXI_SYSCTRL_BASE + 0x168U,
	.has_ext_res_cal = true,
};

/**
 * @brief Return the board-selected Sunxi UFS variant configuration.
 * @return A constant variant descriptor, or a weak default when no
 *         SoC-specific implementation overrides it.
 */
const struct sunxi_ufs_variant *sunxi_ufs_get_variant(void)
{
	return &sun60iw2_ufs_variant;
}

/**
 * @brief Read and decode the UFS PHY calibration words from eFuse SRAM.
 * @param[out] cal Receives the decoded PLL and lane calibration values.
 * @return Zero on success, otherwise a UFS host-controller error code.
 */
int sunxi_get_cal_words(struct sunxi_ufs_cal_words *cal)
{
	const struct sunxi_ufs_variant *variant = sunxi_ufs_get_variant();
	sunxi_sid_t sid;
	uint32_t low;
	uint32_t high;
	int ret;

	if (!cal || !variant) {
		printk_error("UFS PHY: invalid calibration request\n");
		return UFSHC_ERR_INVALID;
	}
	ret = sunxi_sid_dt_read_alias(&sid, "sid0");
	if (ret != DRIVER_OK) {
		printk_error("UFS PHY: SID configuration failed ret=%d\n", ret);
		return UFSHC_ERR_INVALID;
	}
	low = sunxi_efuse_sram_read(&sid, variant->cal_low_offset);
	high = sunxi_efuse_sram_read(&sid, variant->cal_high_offset);
	ufs_debug("UFS PHY: calibration words low=0x%08x high=0x%08x\n", low, high);
	ret = sunxi_decode_cal_words(cal, low, high);
	if (ret)
		printk_error("UFS PHY: calibration decode failed ret=%d\n", ret);
	else
		ufs_debug("UFS PHY: calibration PLL(A/B)=0x%02x/0x%02x AFE0(att/ctle)=0x%02x/0x%02x "
			"AFE1(att/ctle)=0x%02x/0x%02x\n", cal->pll_rate_a, cal->pll_rate_b,
			cal->att_lane0, cal->ctle_lane0, cal->att_lane1, cal->ctle_lane1);
	return ret;
}
