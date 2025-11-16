/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __G_REG_NCAT_H__
#define __G_REG_NCAT_H__

#ifdef __cplusplus
extern "C" {
#endif// __cplusplus

#if defined(CONFIG_CHIP_SUN8IW21)
#include <sun8iw21/reg-ncat.h>
#elif defined(CONFIG_CHIP_SUN8IW20)
#include <sun8iw20/reg-ncat.h>
#elif defined(CONFIG_CHIP_SUN20IW1)
#include <sun20iw1/reg-ncat.h>
#elif defined(CONFIG_CHIP_SUN50IW9)
#include <sun50iw9/reg-ncat.h>
#elif defined(CONFIG_CHIP_SUN55IW3)
#include <sun55iw3/reg-ncat.h>
#elif defined(CONFIG_CHIP_SUN50IW10)
#include <sun50iw10/reg-ncat.h>
#elif defined(CONFIG_CHIP_SUN300IW1)
#include <sun300iw1/reg-ncat.h>
#elif defined(CONFIG_CHIP_SUN60IW2)
#include <sun60iw2/reg-ncat.h>
#elif defined(CONFIG_CHIP_SUN55IW6)
#include <sun55iw6/reg-ncat.h>
#elif defined(CONFIG_CHIP_SUN252IW1)
#include <sun252iw1/reg-ncat.h>
#elif defined(CONFIG_CHIP_SUN65IW1)
#include <sun65iw1/reg-ncat.h>
#else
#error "Unsupported chip"
#endif

#ifdef __cplusplus
}
#endif// __cplusplus

#endif// __G_REG_NCAT_H__