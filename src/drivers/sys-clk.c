/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file sys-clk.c
 * @brief System clock driver for Allwinner (sunxi) platforms
 * @details This file provides weak implementations of clock-related functions
 *          that can be overridden by platform-specific code.
 */

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>

/**
 * @brief Current high-speed oscillator frequency
 * @details This external variable holds the current frequency of the high-speed
 *          oscillator (HOSC) in MHz.
 */
extern uint32_t current_hosc_freq;

/**
 * @brief Initialize system clocks
 * @details This weak function initializes all system clocks. Platform-specific
 *          implementations should override this function.
 */
void __attribute__((weak)) sunxi_clk_init(void) {
}

/**
 * @brief Pre-initialize system clocks
 * @details This weak function performs preliminary clock initialization before
 *          the main clock initialization. Platform-specific implementations
 *          should override this function.
 */
void __attribute__((weak)) sunxi_clk_pre_init(void) {
}

/**
 * @brief Get high-speed oscillator type
 * @details This weak function returns the type/frequency of the high-speed
 *          oscillator (HOSC). The default implementation returns 24 MHz.
 * @return High-speed oscillator frequency in MHz
 */
uint32_t __attribute__((weak)) sunxi_clk_get_hosc_type() {
	return 24;
}

/**
 * @brief Reset system clocks
 * @details This weak function resets system clocks to their default state.
 *          Platform-specific implementations should override this function.
 */
void __attribute__((weak)) sunxi_clk_reset(void) {
}

/**
 * @brief Dump clock information
 * @details This weak function dumps information about the current clock
 *          configuration for debugging purposes. Platform-specific
 *          implementations should override this function.
 */
void __attribute__((weak)) sunxi_clk_dump(void) {
}

/**
 * @brief Deinitialize USB clocks
 * @details This weak function deinitializes clocks for USB controllers.
 *          Platform-specific implementations should override this function.
 */
void __attribute__((weak)) sunxi_usb_clk_deinit(void) {
}

/**
 * @brief Initialize USB clocks
 * @details This weak function initializes clocks for USB controllers.
 *          Platform-specific implementations should override this function.
 */
void __attribute__((weak)) sunxi_usb_clk_init(void) {
}

/**
 * @brief Get peripheral 1x clock rate
 * @details This weak function returns the frequency of the peripheral 1x clock.
 *          The default implementation returns 0.
 * @return Peripheral 1x clock frequency in Hz
 */
uint32_t __attribute__((weak)) sunxi_clk_get_peri1x_rate() {
	return 0;
}

/**
 * @brief Set CPU PLL frequency
 * @details This weak function configures the CPU PLL to the specified frequency.
 *          Platform-specific implementations should override this function.
 * @param freq Target frequency in Hz
 */
void __attribute__((weak)) sunxi_clk_set_cpu_pll(uint32_t freq) {
}
