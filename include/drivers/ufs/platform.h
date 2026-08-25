/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __SYTERKIT_UFS_PLATFORM_H__
#define __SYTERKIT_UFS_PLATFORM_H__

#include <drivers/ufs/ufshc.h>

/* Compatibility include for board code.  Concrete host implementations live
 * below drivers/ufs/host and are selected through drivers/ufs/host/Kconfig. */
extern const struct ufshc_platform_ops ufs_platform_noop;

#endif /* __SYTERKIT_UFS_PLATFORM_H__ */
