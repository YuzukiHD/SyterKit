/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file timer.h
 * @brief ARM architectural timer and busy-wait delay API.
 */

#ifndef __SYS_TIMER_H__
#define __SYS_TIMER_H__

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>

#include <types.h>

#include "log.h"

/**
 * @brief Read the ARM architectural counter.
 *
 * The value is a monotonic 24-MHz tick count while the architectural timer is
 * enabled. Callers normally use ::time_us or ::time_ms for converted units.
 *
 * @return Current counter value in hardware ticks.
 */
uint64_t get_arch_counter(void);

/**
 * @brief Return elapsed time in milliseconds.
 * @return Counter time converted using the ARM 24-MHz timer frequency.
 */
uint32_t time_ms(void);

/**
 * @brief Return elapsed time in microseconds.
 * @return Counter time converted using the ARM 24-MHz timer frequency.
 */
uint64_t time_us(void);

/**
 * @brief Busy-wait for a number of microseconds.
 * @param[in] us Delay duration in microseconds.
 */
void udelay(uint32_t us);

/**
 * @brief Busy-wait for a number of milliseconds.
 * @param[in] ms Delay duration in milliseconds.
 */
void mdelay(uint32_t ms);

/**
 * @brief Busy-wait for an implementation-specific loop count.
 * @param[in] loops Number of decrement-and-branch iterations.
 */
void sdelay(uint32_t loops);

/**
 * @brief Return the timestamp captured during timer initialization.
 * @return Initialization time in microseconds, used as the log epoch.
 */
uint32_t get_init_timestamp();

#endif // __SYS_TIMER_H__
