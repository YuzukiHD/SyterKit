/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __OS_H__
#define __OS_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <timer.h>

#define TIMER_ALWAYS_RUN 0xFFFFFFFF

typedef struct task_struct {
	void (*callback)(void *arg, uint32_t event);
	void *arg;
	uint32_t run_count;
	uint32_t max_run_count;
	uint32_t interval;
	uint32_t elapsed_time;
	struct task_struct *next;
} task_t;

typedef struct timer_struct {
	task_t task;
	uint32_t interval;
} timer_t;

/**
 * @brief Initialize a timer object.
 *
 * @param[out] timer Timer object to initialize.
 * @param[in] callback Function invoked whenever the timer expires.
 * @param[in] arg Opaque argument passed to @p callback.
 */
void timer_create(timer_t *timer, void (*callback)(void *arg, uint32_t event), void *arg);

/**
 * @brief Start a timer.
 *
 * @param[in,out] timer Initialized timer object.
 * @param[in] max_run_count Maximum number of expirations, or zero to run
 *                          indefinitely.
 * @param[in] interval Number of timer-handler ticks between expirations.
 */
void timer_start(timer_t *timer, uint32_t max_run_count, uint32_t interval);

/**
 * @brief Advance and dispatch all active software timers.
 *
 * Call this function once per scheduler tick.
 */
void timer_handle(void);

#endif // __OS_H__
