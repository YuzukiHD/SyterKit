/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __SYS_TIMER_H__
#define __SYS_TIMER_H__

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>

#include <types.h>

#include "log.h"

/**
 * @brief Get the architecture-specific counter value.
 *
 * This function retrieves the current value of the architecture-specific
 * timer counter. The returned value is a 64-bit integer representing the
 * timer's current count.
 *
 * @return Current counter value as a 64-bit integer.
 */
uint64_t get_arch_counter(void);

/**
 * @brief Get the current time in milliseconds.
 *
 * This function calculates the current time based on the architecture
 * counter and returns it in milliseconds.
 *
 * @return Current time in milliseconds.
 */
uint32_t time_ms(void);

/**
 * @brief Get the current time in microseconds.
 *
 * This function calculates the current time based on the architecture
 * counter and returns it in microseconds.
 *
 * @return Current time in microseconds.
 */
uint64_t time_us(void);

/**
 * @brief Delay execution for a specified number of microseconds.
 *
 * This function creates a delay for the specified number of microseconds
 * using a busy-wait loop.
 *
 * @param us Number of microseconds to delay.
 */
void udelay(uint32_t us);

/**
 * @brief Delay execution for a specified number of milliseconds.
 *
 * This function converts milliseconds to microseconds and calls the
 * udelay function to implement the delay.
 *
 * @param ms Number of milliseconds to delay.
 */
void mdelay(uint32_t ms);

/**
 * @brief Delay execution for a specified number of loops (treated as microseconds).
 *
 * This function creates a delay based on the number of microsecond loops
 * specified, by calling the udelay function directly.
 *
 * @param loops Number of microsecond loops to delay.
 */
void sdelay(uint32_t loops);

/**
 * @brief Get the initialization timestamp.
 *
 * This function retrieves the initialization timestamp set during
 * the system timer setup.
 *
 * @return The initialization timestamp in microseconds.
 */
uint32_t get_init_timestamp();

#endif// __SYS_TIMER_H__