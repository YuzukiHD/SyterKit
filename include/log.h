/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <timer.h>

#include <drivers/serial/serial.h>

#include "format.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define LOG_LEVEL_MUTE 0
#define LOG_LEVEL_ERROR 1
#define LOG_LEVEL_WARNING 2
#define LOG_LEVEL_INFO 3
#define LOG_LEVEL_DEBUG 4
#define LOG_LEVEL_TRACE 5
#define LOG_LEVEL_BACKTRACE 6

#ifndef LOG_LEVEL_DEFAULT

#ifdef DEBUG_MODE
#define LOG_LEVEL_DEFAULT LOG_LEVEL_DEBUG
#elif defined TRACE_MODE
#define LOG_LEVEL_DEFAULT LOG_LEVEL_TRACE
#else
#define LOG_LEVEL_DEFAULT LOG_LEVEL_INFO
#endif // DEBUG_MODE

#endif // LOG_LEVEL_DEFAULT

/* Driver subdirectories may override the global threshold per build unit. */
#ifdef DRIVER_LOG_LEVEL
#undef LOG_LEVEL_DEFAULT
#define LOG_LEVEL_DEFAULT DRIVER_LOG_LEVEL
#endif

#define no_printk(level, fmt, ...)                         \
	do {                                               \
		if (0)                                     \
			printk(level, fmt, ##__VA_ARGS__); \
	} while (0)

#if LOG_LEVEL_DEFAULT >= LOG_LEVEL_TRACE
#define pr_trace(fmt, ...) printk(LOG_LEVEL_TRACE, fmt, ##__VA_ARGS__)
#else
#define pr_trace(fmt, ...) no_printk(LOG_LEVEL_TRACE, fmt, ##__VA_ARGS__)
#endif

#if LOG_LEVEL_DEFAULT >= LOG_LEVEL_DEBUG
#define pr_debug(fmt, ...) printk(LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
#else
#define pr_debug(fmt, ...) no_printk(LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
#endif

#if LOG_LEVEL_DEFAULT >= LOG_LEVEL_INFO
#define pr_info(fmt, ...) printk(LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
#else
#define pr_info(fmt, ...) no_printk(LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
#endif

#if LOG_LEVEL_DEFAULT >= LOG_LEVEL_WARNING
#define pr_warn(fmt, ...) printk(LOG_LEVEL_WARNING, fmt, ##__VA_ARGS__)
#else
#define pr_warn(fmt, ...) no_printk(LOG_LEVEL_WARNING, fmt, ##__VA_ARGS__)
#endif

#if LOG_LEVEL_DEFAULT >= LOG_LEVEL_ERROR
#define pr_err(fmt, ...) printk(LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)
#else
#define pr_err(fmt, ...) no_printk(LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)
#endif

/**
 * @brief Set timer count
 * 
 * This function is used to set the count value of the timer.
 * 
 * @note Before calling this function, timer-related hardware configuration should be initialized.
 */
void set_timer_count();

/**
 * @brief Print message to kernel log
 * 
 * This function is used to print the message with specified level to the kernel log.
 * 
 * @param level Information level, typically used to specify the importance or type of the message.
 * @param fmt   Format string describing the format of the message to print.
 * @param ...   Variable argument list used to fill placeholders in the format string.
 * 
 * @note This function should be used within the kernel to record important system information or debug messages.
 */
void printk(int level, const char *fmt, ...);

/**
 * @brief Print message via UART
 * 
 * This function is used to print formatted message to the terminal via UART serial port.
 * 
 * @param fmt Format string describing the format of the message to print.
 * @param ... Variable argument list used to fill placeholders in the format string.
 * 
 * @note This function is typically used in embedded systems for debugging and outputting system status information.
 */
void uart_printf(const char *fmt, ...);

/**
 * @brief Print message via UART
 * 
 * This function is used to print formatted message to the terminal via UART serial port.
 * 
 * @param fmt Format string describing the format of the message to print.
 * @param ... Variable argument list used to fill placeholders in the format string.
 * 
 * @note This function is typically used in embedded systems for debugging and outputting system status information.
 */
int printf(const char *fmt, ...);

/**
 * @brief Dumps memory content in hexadecimal format.
 *
 * This function dumps the content of memory starting from the specified
 * address in hexadecimal format. It prints the hexadecimal values of memory
 * contents and ASCII characters corresponding to printable characters.
 *
 * @param start_addr The starting address of the memory region to dump.
 * @param count The number of bytes to dump.
 */
void dump_hex(uintptr_t start_addr, uint32_t count);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif
