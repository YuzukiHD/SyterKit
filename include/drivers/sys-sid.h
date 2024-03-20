/* SPDX-License-Identifier: Apache-2.0 */

#ifndef _SYS_SID_H_
#define _SYS_SID_H_

#ifdef __cplusplus
extern "C" {
#endif// __cplusplus

#if defined(CONFIG_CHIP_SUN8IW21)
    #include <sun8iw21/sys-sid.h>
#elif defined(CONFIG_CHIP_SUN8IW20)
    #include <sun8iw20/sys-sid.h>
#elif defined(CONFIG_CHIP_SUN20IW1)
    #include <sun20iw1/sys-sid.h>
#elif defined(CONFIG_CHIP_SUN50IW9)
    #include <sun50iw9/sys-sid.h>
#elif defined(CONFIG_CHIP_SUN55IW3)
    #include <sun55iw3/sys-sid.h>

#elif defined(CONFIG_CHIP_SUN50IW10)
    #include <sun50iw10/sys-sid.h>
#else
    #error "Unsupported chip"
#endif

#ifdef __cplusplus
}
#endif // __cplusplus

#endif// _SYS_SID_H_