/* SPDX-License-Identifier: Apache-2.0 */

#ifndef __SYS_RPROC_H__
#define __SYS_RPROC_H__

#if defined(CONFIG_CHIP_SUN8IW21)
#include <sun8iw21/sys-rproc.h>
#else
#error "Unsupported chip"
#endif

#endif// __SYS_RPROC_H__