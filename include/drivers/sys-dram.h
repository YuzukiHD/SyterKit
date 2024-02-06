/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __SYS_DRAM_H__
#define __SYS_DRAM_H__

#ifdef __cplusplus
extern "C" {
#endif// __cplusplus

#if defined(CONFIG_CHIP_SUN8IW21)
    #include <sun8iw21/sys-dram.h>
#elif defined(CONFIG_CHIP_SUN8IW20)
    #include <sun8iw20/sys-dram.h>
#elif defined(CONFIG_CHIP_SUN20IW1)
    #include <sun20iw1/sys-dram.h>
#elif defined(CONFIG_CHIP_SUN50IW9)
    #include <sun50iw9/sys-dram.h>
#elif defined(CONFIG_CHIP_SUN55IW3)
    #include <sun55iw3/sys-dram.h>
#elif defined(CONFIG_CHIP_SUN8IW8)
    #include <sun8iw8/sys-dram.h>
#elif defined(CONFIG_CHIP_SUN50IW10)
    #include <sun50iw10/sys-dram.h>
#else
    #error "Unsupported chip"
#endif

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __SYS_DRAM_H__
