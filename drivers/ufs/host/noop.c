/* SPDX-License-Identifier: GPL-2.0+ */

#include <drivers/ufs/host.h>
#include <drivers/ufs/host/sunxi.h>

const struct ufshc_platform_ops ufs_platform_noop = {
	.enable = 0,
	.disable = 0,
	.phy_init = 0,
	.get_ref_clk_freq = 0,
	.prepare = 0,
	.device_reset = 0,
	.link_startup = 0,
	.link_up = 0,
	.priv = 0,
};

const struct ufshc_platform_ops *ufs_platform_default(void)
{
#if defined(CONFIG_DRIVER_UFS_HOST_SUN60IW2)
	return &sunxi_ufs_platform_ops;
#else
	return &ufs_platform_noop;
#endif
}
