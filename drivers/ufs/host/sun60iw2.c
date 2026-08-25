/* SPDX-License-Identifier: GPL-2.0+ */

#include <drivers/clk/sun60iw2/reg.h>
#include <drivers/soc/sid.h>
#include <drivers/ufs/host/sunxi.h>
#include <dt-compatible/sid-dt.h>

static const struct sunxi_ufs_variant sun60iw2_ufs_variant = {
	.cal_low_offset = 0x60U,
	.cal_high_offset = 0x64U,
	.ext_res_ctrl = SUNXI_SYSCTRL_BASE + 0x160U,
	.ext_res1_ctrl = SUNXI_SYSCTRL_BASE + 0x168U,
	.has_ext_res_cal = true,
};

const struct sunxi_ufs_variant *sunxi_ufs_get_variant(void)
{
	return &sun60iw2_ufs_variant;
}

int sunxi_get_cal_words(struct sunxi_ufs_cal_words *cal)
{
	const struct sunxi_ufs_variant *variant = sunxi_ufs_get_variant();
	sunxi_sid_t sid;

	if (!cal || !variant || sunxi_sid_dt_read_alias(&sid, "sid0") != DRIVER_OK)
		return UFSHC_ERR_INVALID;
	return sunxi_decode_cal_words(cal,
		sunxi_efuse_sram_read(&sid, variant->cal_low_offset),
		sunxi_efuse_sram_read(&sid, variant->cal_high_offset));
}
