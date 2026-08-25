/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __SYTERKIT_UFS_HOST_H__
#define __SYTERKIT_UFS_HOST_H__

#include <drivers/ufs/ufshc.h>

/*
 * Return the platform selected by CONFIG_SOC_* for a DT-described UFS
 * controller.  Board code can still provide a private ops table when a
 * board has external regulators or a non-standard reset topology.
 */
const struct ufshc_platform_ops *ufs_platform_default(void);

/* Exported for board code that needs to select a specific host explicitly. */
extern const struct ufshc_platform_ops ufs_platform_noop;
extern const struct ufshc_platform_ops sunxi_ufs_platform_ops;

#endif /* __SYTERKIT_UFS_HOST_H__ */
