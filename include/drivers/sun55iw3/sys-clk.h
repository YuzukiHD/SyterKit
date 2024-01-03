#ifndef __SUN55IW3_CLK_H__
#define __SUN55IW3_CLK_H__

#include "reg/reg-ccu.h"

/**
 * @brief Initialize the global clocks.
 *
 * This function initializes the global clocks, including PLLs and clock dividers.
 */
void sunxi_clk_init(void);

/**
 * @brief Reset the global clocks.
 *
 * This function resets all global clocks to their default values.
 */
void sunxi_clk_reset(void);

/**
 * @brief Dump all clock-related register values.
 *
 * This function prints out all clock-related register values for debugging and observation.
 */
void sunxi_clk_dump(void);

/**
 * @brief Get the clock rate of the PERI1X bus.
 *
 * @return The clock rate of the PERI1X bus in Hz.
 */
uint32_t sunxi_clk_get_peri1x_rate();


#endif// __SUN55IW3_CLK_H__