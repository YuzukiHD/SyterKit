/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __G_REG_NCAT_H__
#define __G_REG_NCAT_H__

#ifdef __cplusplus
extern "C" {
#endif// __cplusplus

#if defined(CONFIG_SOC_SUN8IW21)
#include <drivers/soc/sun8iw21/reg-ncat.h>
#elif defined(CONFIG_SOC_SUN8IW20)
#include <drivers/soc/sun8iw20/reg-ncat.h>
#elif defined(CONFIG_SOC_SUN20IW1)
#include <drivers/soc/sun20iw1/reg-ncat.h>
#elif defined(CONFIG_SOC_SUN50IW9)
#include <drivers/soc/sun50iw9/reg-ncat.h>
#elif defined(CONFIG_SOC_SUN55IW3)
#include <drivers/soc/sun55iw3/reg-ncat.h>
#elif defined(CONFIG_SOC_SUN50IW10)
#include <drivers/soc/sun50iw10/reg-ncat.h>
#elif defined(CONFIG_SOC_SUN300IW1)
#include <drivers/soc/sun300iw1/reg-ncat.h>
#elif defined(CONFIG_SOC_SUN60IW2)
#include <drivers/soc/sun60iw2/reg-ncat.h>
#elif defined(CONFIG_SOC_SUN55IW6)
#include <drivers/soc/sun55iw6/reg-ncat.h>
#elif defined(CONFIG_SOC_SUN252IW1)
#include <drivers/soc/sun252iw1/reg-ncat.h>
#elif defined(CONFIG_SOC_SUN65IW1)
#include <drivers/soc/sun65iw1/reg-ncat.h>
#elif defined(CONFIG_SOC_SUN8IW22)
#include <drivers/soc/sun8iw22/reg-ncat.h>
#else
#error "Unsupported chip"
#endif

#ifdef __cplusplus
}
#endif// __cplusplus

#endif// __G_REG_NCAT_H__