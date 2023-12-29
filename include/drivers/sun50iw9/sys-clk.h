#ifndef __SUN50IW9_CLK_H__
#define __SUN50IW9_CLK_H__

#include "reg/reg-ccu.h"

#ifdef __cplusplus
extern "C" { 
#endif // __cplusplus

void sunxi_clk_init(void);

void sunxi_clk_reset(void);

void sunxi_clk_dump(void);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif// __SUN50IW9_CLK_H__