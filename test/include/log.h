/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __SYTER_TEST_LOG_H__
#define __SYTER_TEST_LOG_H__

#include <stdint.h>

#include <io.h>

#define LOG_LEVEL_MUTE 0
#define LOG_LEVEL_ERROR 1
#define LOG_LEVEL_WARNING 2
#define LOG_LEVEL_INFO 3
#define LOG_LEVEL_DEBUG 4
#define LOG_LEVEL_TRACE 5
#define LOG_LEVEL_BACKTRACE 6

#ifndef LOG_LEVEL_DEFAULT
#define LOG_LEVEL_DEFAULT LOG_LEVEL_INFO
#endif

#define no_printk(level, fmt, ...) \
	do { \
		if (0) \
			printk(level, fmt, ##__VA_ARGS__); \
	} while (0)

#if LOG_LEVEL_DEFAULT >= LOG_LEVEL_TRACE
#define printk_trace(fmt, ...) printk(LOG_LEVEL_TRACE, fmt, ##__VA_ARGS__)
#else
#define printk_trace(fmt, ...) no_printk(LOG_LEVEL_TRACE, fmt, ##__VA_ARGS__)
#endif

#if LOG_LEVEL_DEFAULT >= LOG_LEVEL_DEBUG
#define printk_debug(fmt, ...) printk(LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
#else
#define printk_debug(fmt, ...) no_printk(LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
#endif

#define printk_info(fmt, ...) printk(LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
#define printk_warning(fmt, ...) printk(LOG_LEVEL_WARNING, fmt, ##__VA_ARGS__)
#define printk_error(fmt, ...) printk(LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)

void printk(int level, const char *fmt, ...);
void uart_printf(const char *fmt, ...);
int printf(const char *fmt, ...);
void dump_hex(uintptr_t start_addr, uint32_t count);

#endif
