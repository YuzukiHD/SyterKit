/* SPDX-License-Identifier: Apache-2.0 */

#ifndef __SUN8IW21_SYS_RPROC_H__
#define __SUN8IW21_SYS_RPROC_H__

#include "reg/reg-rproc.h"

#ifdef __cplusplus
extern "C" { 
#endif // __cplusplus

void sunxi_e907_clock_init(uint32_t addr);

void sunxi_e907_clock_reset(void);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif// __SUN8IW21_SYS_RPROC_H__