#ifndef __SYS_CLK_H__
#define __SYS_CLK_H__

#if defined(CONFIG_CHIP_SUN8IW21)
#include <sun8iw21/sys-clk.h>
#else
#error "Unsupported chip"
#endif

#endif // __SYS_CLK_H__