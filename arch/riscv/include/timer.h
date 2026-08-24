/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file timer.h
 * @brief RISC-V timer frequency detection, clocks, and delay API.
 */

#ifndef __SYS_TIMER_H__
#define __SYS_TIMER_H__

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include "log.h"

extern uint32_t current_hosc_freq;

void set_timer_count(void);

int sunxi_hosc_detect(void);

/**
 * @brief Read a coherent architectural time counter.
 * @return Current RISC-V @c time CSR value in hardware ticks.
 */
uint64_t get_arch_counter(void);

/**
 * @brief Convert the architectural counter to milliseconds.
 * @return Elapsed milliseconds using the detected oscillator frequency.
 */
uint32_t time_ms(void);

/**
 * @brief Convert the architectural counter to microseconds.
 * @return Elapsed microseconds using the detected oscillator frequency.
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

void sdelay(uint32_t loops);

/**
 * @brief Return the timestamp captured by ::set_timer_count.
 * @return Initialization time in microseconds.
 */
uint32_t get_init_timestamp();

#endif // __SYS_TIMER_H__
