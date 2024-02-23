/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __G_REG_SMHC_H__
#define __G_REG_SMHC_H__

#ifdef __cplusplus
extern "C" {
#endif// __cplusplus

#if defined(CONFIG_CHIP_SUN8IW21)
    #include <sun8iw21/reg/reg-smhc.h>
#elif defined(CONFIG_CHIP_SUN8IW20)
    #include <sun8iw20/reg/reg-smhc.h>
#elif defined(CONFIG_CHIP_SUN20IW1)
    #include <sun20iw1/reg/reg-smhc.h>
#elif defined(CONFIG_CHIP_SUN50IW9)
    #include <sun50iw9/reg/reg-smhc.h>
#elif defined(CONFIG_CHIP_SUN55IW3)
    #include <sun55iw3/reg/reg-smhc.h>
#elif defined(CONFIG_CHIP_SUN8IW8)
    #include <sun8iw8/reg/reg-smhc.h>
#elif defined(CONFIG_CHIP_SUN50IW10)
    #include <sun50iw10/reg/reg-smhc.h>
#else
    #error "Unsupported chip"
#endif

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __G_REG_SMHC_H__