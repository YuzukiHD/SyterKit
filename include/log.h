/* SPDX-License-Identifier: MIT */

#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <timer.h>

#include <sys-uart.h>

#include "xformat.h"

#ifdef __cplusplus
extern "C" {
#endif// __cplusplus

#define LOG_LEVEL_TRACE 0
#define LOG_LEVEL_DEBUG 1
#define LOG_LEVEL_INFO 2
#define LOG_LEVEL_WARNING 3
#define LOG_LEVEL_ERROR 4
#define LOG_LEVEL_MUTE 5

#ifndef LOG_LEVEL_DEFAULT

#ifdef DEBUG_MODE
#define LOG_LEVEL_DEFAULT LOG_LEVEL_DEBUG
#elif defined TRACE_MODE
#define LOG_LEVEL_DEFAULT LOG_LEVEL_TRACE
#else
#define LOG_LEVEL_DEFAULT LOG_LEVEL_INFO
#endif// DEBUG_MODE

#endif// LOG_LEVEL_DEFAULT

#if LOG_LEVEL_DEFAULT >= LOG_LEVEL_TRACE
#define printk_trace(fmt, ...) printk(LOG_LEVEL_TRACE, fmt, ##__VA_ARGS__)
#else
#define printk_trace(fmt, ...) ((void) 0)
#endif

#if LOG_LEVEL_DEFAULT >= LOG_LEVEL_DEBUG
#define printk_debug(fmt, ...) printk(LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
#else
#define printk_debug(fmt, ...) ((void) 0)
#endif

#if LOG_LEVEL_DEFAULT >= LOG_LEVEL_INFO
#define printk_info(fmt, ...) printk(LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
#else
#define printk_info(fmt, ...) ((void) 0)
#endif

#if LOG_LEVEL_DEFAULT >= LOG_LEVEL_WARNING
#define printk_warning(fmt, ...) printk(LOG_LEVEL_WARNING, fmt, ##__VA_ARGS__)
#else
#define printk_warning(fmt, ...) ((void) 0)
#endif

#if LOG_LEVEL_DEFAULT >= LOG_LEVEL_ERROR
#define printk_error(fmt, ...) printk(LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)
#else
#define printk_error(fmt, ...) ((void) 0)
#endif

void set_timer_count();

void printk(int level, const char *fmt, ...);

void uart_printf(const char *fmt, ...);

#ifdef __cplusplus
}
#endif// __cplusplus

#endif
