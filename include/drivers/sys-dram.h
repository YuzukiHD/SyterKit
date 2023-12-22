/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __SYS_DRAM_H__
#define __SYS_DRAM_H__

#if defined(CONFIG_CHIP_SUN8IW21)
#include <sun8iw21/sys-dram.h>
#else
#error "Unsupported chip"
#endif

#endif // __SYS_DRAM_H__
