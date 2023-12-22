/* SPDX-License-Identifier: Apache-2.0 */

#ifndef _SYS_SID_H_
#define _SYS_SID_H_

#if defined(CONFIG_CHIP_SUN8IW21)
#include <sun8iw21/sys-sid.h>
#else
#error "Unsupported chip"
#endif

#endif// _SYS_SID_H_