#ifndef __SYS_CLK_H__
#define __SYS_CLK_H__

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include "reg/reg-ccu.h"

#ifdef __cplusplus
extern "C" {
#endif// __cplusplus

typedef struct {
	uint32_t gate_reg_base;
	uint32_t gate_reg_offset;
	uint32_t rst_reg_base;
	uint32_t rst_reg_offset;
	uint32_t parent_clk;
} sunxi_clk_t;

/**
 * @brief Initialize the global clocks.
 *
 * This function initializes the global clocks, including PLLs and clock dividers.
 */
void sunxi_clk_init(void);

/**
 * @brief Initialize the necessary clocks for minsys boot up
 *
 * This function initializes the necessary clocks for minsys boot up clocks
 */
void sunxi_clk_pre_init(void);

/**
 * @brief Get the type of High-Speed Oscillator (HOSC).
 *
 * This function retrieves the type of the High-Speed Oscillator currently being used.
 * The returned value can indicate different HOSC configurations or features supported 
 * by the system.
 *
 * @return uint32_t Type of the HOSC.
 */
uint32_t sunxi_clk_get_hosc_type(void);

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

/**
 * @brief Change the cpu freq
 * 
 * @param freq The freq of cpu want to set.
 */
void sunxi_clk_set_cpu_pll(uint32_t freq);

#ifdef __cplusplus
}
#endif// __cplusplus

#endif// __SYS_CLK_H__