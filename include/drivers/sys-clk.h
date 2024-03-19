#ifndef __SYS_CLK_H__
#define __SYS_CLK_H__

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#ifdef __cplusplus
extern "C" {
#endif// __cplusplus

#if defined(CONFIG_CHIP_SUN8IW21)
    #include <sun8iw21/sys-clk.h>
#elif defined(CONFIG_CHIP_SUN8IW20)
    #include <sun8iw20/sys-clk.h>
#elif defined(CONFIG_CHIP_SUN20IW1)
    #include <sun20iw1/sys-clk.h>
#elif defined(CONFIG_CHIP_SUN50IW9)
    #include <sun50iw9/sys-clk.h>
#elif defined(CONFIG_CHIP_SUN55IW3)
    #include <sun55iw3/sys-clk.h>
#elif defined(CONFIG_CHIP_SUN8IW8)
    #include <sun8iw8/sys-clk.h>
#elif defined(CONFIG_CHIP_SUN50IW10)
    #include <sun50iw10/sys-clk.h>
#else
    #error "Unsupported chip"
#endif

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

/**
 * @brief Get the clock rate of the PERI1X bus.
 *
 * @return The clock rate of the PERI1X bus in Hz.
 */
uint32_t sunxi_clk_get_peri1x_rate();

/**
 * @brief Deinitialize USB clock.
 */
void sunxi_usb_clk_deinit();

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __SYS_CLK_H__