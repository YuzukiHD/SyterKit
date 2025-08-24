/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <timer.h>

#include "os.h"

static task_t *task_list = NULL;

// Add a new task to the linked list
static void add_task(task_t *new_task) {
	new_task->run_count = 0;   // Set the run count of the new task to 0
	new_task->elapsed_time = 0;// Set the elapsed time of the new task to 0
	new_task->next = NULL;	   // Set the next pointer of the new task to NULL

	// If the task list is empty, set the new task as the head of the list
	if (task_list == NULL) {
		task_list = new_task;
	} else {// Otherwise, traverse the list and append the new task to the end
		task_t *cur_task = task_list;
		while (cur_task->next != NULL) { cur_task = cur_task->next; }
		cur_task->next = new_task;
	}
}

// Remove a given task from the linked list
static void remove_task(task_t *task) {
	task_t *cur_task = task_list;// Initialize a current task pointer to the head of the list
	task_t *prev_task = NULL;	 // Initialize a previous task pointer to NULL

	// Traverse the list to find the task to be removed
	while (cur_task != NULL) {
		if (cur_task == task) {
			// If the task is the head, set the next task as the new head
			if (prev_task == NULL) {
				task_list = cur_task->next;
			} else {// Otherwise, remove the task by connecting the previous and next tasks
				prev_task->next = cur_task->next;
			}
			break;
		}

		prev_task = cur_task;
		cur_task = cur_task->next;
	}
}

// Loop through all tasks in the linked list and execute them if needed
static void task_loop() {
	task_t *cur_task = task_list;// Initialize a current task pointer to the head of the list

	while (cur_task != NULL) {
		cur_task->elapsed_time += 1;// Increment the elapsed time of the current task by 1 (assuming a timeout of 1 unit)

		if (cur_task->elapsed_time >= cur_task->interval) {
			cur_task->callback(cur_task->arg, cur_task->run_count);// Execute the callback function of the current task with its arguments
			cur_task->elapsed_time = 0;							   // Reset the elapsed time of the current task to 0
			cur_task->run_count++;								   // Increment the run count of the current task

			// If the current task has reached its maximum run count and it is not set to always run, remove it from the list
			if (cur_task->run_count >= cur_task->max_run_count && cur_task->max_run_count != TIMER_ALWAYS_RUN) {
				remove_task(cur_task);
			}
		}

		cur_task = cur_task->next;// Move to the next task in the list
	}
}

// Create a timer with the given callback function and argument
void timer_create(timer_t *timer, void (*callback)(void *arg, uint32_t event), void *arg) {
	timer->task.callback = callback;// Set the callback function of the timer's task to the given callback function
	timer->task.arg = arg;			// Set the argument of the timer's task to the given argument
	timer->task.max_run_count = 0;	// Set the maximum run count of the timer's task to 0 (default)
	timer->task.interval = 0;		// Set the interval of the timer's task to 0 (default)
	timer->interval = 0;			// Set the interval of the timer to 0 (default)
}

// Start the timer with the given maximum run count and interval
void timer_start(timer_t *timer, uint32_t max_run_count, uint32_t interval) {
	timer->task.max_run_count = max_run_count;// Set the maximum run count of the timer's task to the given maximum run count
	timer->task.interval = interval;		  // Set the interval of the timer's task to the given interval
	timer->interval = interval;				  // Set the interval of the timer to the given interval
	add_task(&(timer->task));				  // Add the timer's task to the task list
}

void timer_handle() {
	task_loop();
}