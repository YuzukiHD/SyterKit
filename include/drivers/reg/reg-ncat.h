/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __G_REG_NCAT_H__
#define __G_REG_NCAT_H__

#if defined(CONFIG_CHIP_SUN8IW21)
#include <sun8iw21/reg/reg-ncat.h>
#else
#error "Unsupported chip"
#endif

#endif // __G_REG_NCAT_H__