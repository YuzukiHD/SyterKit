#ifndef __REG_CCU_H__
#define __REG_CCU_H__

#ifdef __cplusplus
extern "C" {
#endif// __cplusplus

#if defined(CONFIG_CHIP_SUN8IW21)
#include <sun8iw21/reg-ccu.h>
#elif defined(CONFIG_CHIP_SUN8IW20)
#include <sun8iw20/reg-ccu.h>
#elif defined(CONFIG_CHIP_SUN20IW1)
#include <sun20iw1/reg-ccu.h>
#elif defined(CONFIG_CHIP_SUN50IW9)
#include <sun50iw9/reg-ccu.h>
#elif defined(CONFIG_CHIP_SUN55IW3)
#include <sun55iw3/reg-ccu.h>
#elif defined(CONFIG_CHIP_SUN50IW10)
#include <sun50iw10/reg-ccu.h>
#elif defined(CONFIG_CHIP_SUN20IW5)
#include <sun20iw5/reg-ccu.h>
#else
#error "Unsupported chip"
#endif

#ifdef __cplusplus
}
#endif// __cplusplus

#endif// __REG_CCU_H__