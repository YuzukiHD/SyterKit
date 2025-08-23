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
    @brief Create a timer.
    @param timer A pointer to a timer_t object to save the newly created timer.
    @param callback A pointer to the function that will be called when the timer times out.
    @param arg A void pointer used to pass arguments to the callback function. 
*/
void timer_create(timer_t *timer, void (*callback)(void *arg, uint32_t event), void *arg);


/**
    @brief Start a timer.
    @param timer A pointer to an already created timer_t object.
    @param max_run_count The maximum number of times the timer will run. 0 means it will run indefinitely.
    @param interval The time interval of the timer. 
*/
void timer_start(timer_t *timer, uint32_t max_run_count, uint32_t interval);

/**
    @brief Timer processing function, needs to be called continuously in the main loop to achieve normal operation of the timer. 
*/
void timer_handle();

#endif// __OS_H__
