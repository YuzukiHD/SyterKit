#ifndef __SYS_CLK_H__
#define __SYS_CLK_H__

#ifdef __cplusplus
extern "C" {
#endif// __cplusplus

#if defined(CONFIG_CHIP_SUN8IW21)
    #include <sun8iw21/sys-clk.h>
#elif defined(CONFIG_CHIP_SUN8IW20)
    #include <sun8iw20/sys-clk.h>
#elif defined(CONFIG_CHIP_SUN50IW9)
    #include <sun50iw9/sys-clk.h>
#elif defined(CONFIG_CHIP_SUN55IW3)
    #include <sun55iw3/sys-clk.h>
#elif defined(CONFIG_CHIP_SUN8IW8)
    #include <sun8iw8/sys-clk.h>
#else
    #error "Unsupported chip"
#endif

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __SYS_CLK_H__