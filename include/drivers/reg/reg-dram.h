/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __REG_DRAM_H__
#define __REG_DRAM_H__

#ifdef __cplusplus
extern "C" {
#endif// __cplusplus

#if defined(CONFIG_SOC_SUN8IW21)
#include <drivers/soc/sun8iw21/reg-dram.h>
#elif defined(CONFIG_SOC_SUN8IW20)
#include <drivers/soc/sun8iw20/reg-dram.h>
#elif defined(CONFIG_SOC_SUN20IW1)
#include <drivers/soc/sun20iw1/reg-dram.h>
#elif defined(CONFIG_SOC_SUN300IW1)
#include <drivers/soc/sun300iw1/reg-dram.h>
#endif

#ifdef __cplusplus
}
#endif// __cplusplus

#endif// __REG_DRAM_H__