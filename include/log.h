/* SPDX-License-Identifier: MIT */

#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <types.h>

#include <sys-timer.h>
#include <sys-uart.h>

#include "xformat.h"

enum {
	LOG_LEVEL_TRACE = 0,
	LOG_LEVEL_DEBUG = 1,
	LOG_LEVEL_INFO = 2,
	LOG_LEVEL_WARNING = 3,
	LOG_LEVEL_ERROR = 4,
	LOG_LEVEL_MUTE = 5,
};

#ifndef LOG_LEVEL_DEFAULT
#define LOG_LEVEL_DEFAULT LOG_LEVEL_DEBUG
#endif

void set_timer_count();

void printk(int level, const char* fmt, ...);

void uart_printf(const char *fmt, ...);

#endif
