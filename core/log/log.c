/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file log.c
 * @brief Timestamped logging and hexadecimal memory-dump helpers.
 *
 * Formatting is delegated to the callback-based formatter while this module
 * supplies severity prefixes, elapsed timestamps, and the UART sink.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <timer.h>
#include <types.h>

#include "log.h"
#include "uart.h"
#include "format.h"

/**
 * @brief Write a severity-prefixed, timestamped log message.
 * @param[in] level Log severity from the `LOG_LEVEL_*` constants.
 * @param[in] fmt printf-style format string.
 * @param[in] ... Values consumed by @p fmt.
 *
 * The timestamp is measured from the architecture timer initialization point.
 * Color escape sequences are emitted unless disabled by the build option.
 */
void printk(int level, const char *fmt, ...)
{
	uint32_t now_timestamp = time_us() - get_init_timestamp();
	uint32_t seconds = now_timestamp / (1000 * 1000);
	uint32_t milliseconds = now_timestamp % (1000 * 1000);

#ifdef DISBALE_COLOR_PRINTK
	switch (level) {
	case LOG_LEVEL_TRACE:
		uart_printf("[%5lu.%06lu][T] ", seconds, milliseconds);
		break;
	case LOG_LEVEL_DEBUG:
		uart_printf("[%5lu.%06lu][D] ", seconds, milliseconds);
		break;
	case LOG_LEVEL_INFO:
		uart_printf("[%5lu.%06lu][I] ", seconds, milliseconds);
		break;
	case LOG_LEVEL_WARNING:
		uart_printf("[%5lu.%06lu][W] ", seconds, milliseconds);
		break;
	case LOG_LEVEL_ERROR:
		uart_printf("[%5lu.%06lu][E] ", seconds, milliseconds);
		break;
	case LOG_LEVEL_BACKTRACE:
		uart_printf("[%5lu.%06lu][B] ", seconds, milliseconds);
	case LOG_LEVEL_MUTE:
	default:
		break;
	}
#else
	switch (level) {
	case LOG_LEVEL_TRACE:
		uart_printf("[%5lu.%06lu][\033[30mT\033[0m] ", seconds, milliseconds);
		break;
	case LOG_LEVEL_DEBUG:
		uart_printf("[%5lu.%06lu][\033[32mD\033[0m] ", seconds, milliseconds);
		break;
	case LOG_LEVEL_INFO:
		uart_printf("[%5lu.%06lu][\033[36mI\033[0m] ", seconds, milliseconds);
		break;
	case LOG_LEVEL_WARNING:
		uart_printf("[%5lu.%06lu][\033[33mW\033[0m] ", seconds, milliseconds);
		break;
	case LOG_LEVEL_ERROR:
		uart_printf("[%5lu.%06lu][\033[31mE\033[0m] ", seconds, milliseconds);
		break;
	case LOG_LEVEL_BACKTRACE:
		uart_printf("[%5lu.%06lu][\033[38;5;214mB\033[0m] ", seconds, milliseconds);
	case LOG_LEVEL_MUTE:
	default:
		break;
	}
#endif
	va_list args;
	va_start(args, fmt);
	va_list args_copy;
	va_copy(args_copy, args);
	vformat(uart_log_putchar, NULL, fmt, args_copy);
	va_end(args);
	va_end(args_copy);
}

/**
 * @brief Forward a preformatted message through the generic kernel logger.
 * @param[in] level Log severity from the `LOG_LEVEL_*` constants.
 * @param[in] message NUL-terminated message text.
 */
void printk_string(int level, const char *message)
{
	if (message != NULL)
		printk(level, "%s", message);
}

/**
 * @brief Format a message directly to the UART without a severity prefix.
 * @param[in] fmt printf-style format string.
 * @param[in] ... Values consumed by @p fmt.
 */
void uart_printf(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	va_list args_copy;
	va_copy(args_copy, args);
	vformat(uart_log_putchar, NULL, fmt, args_copy);
	va_end(args);
	va_end(args_copy);
}

/**
 * @brief Provide the freestanding `printf` entry point over the UART.
 * @param[in] fmt printf-style format string.
 * @param[in] ... Values consumed by @p fmt.
 * @return Zero after the formatter has consumed the argument list.
 */
int printf(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	va_list args_copy;
	va_copy(args_copy, args);
	vformat(uart_log_putchar, NULL, fmt, args_copy);
	va_end(args);
	va_end(args_copy);

	return 0;
}

/**
 * @brief Print a timestamped informational message during DRAM bring-up.
 * @param[in] fmt printf-style format string.
 * @param[in] ... Values consumed by @p fmt.
 * @return Zero after the message has been formatted.
 */
int printf_dram(const char *fmt, ...)
{
	uint32_t now_timestamp = time_us() - get_init_timestamp();
	uint32_t seconds = now_timestamp / (1000 * 1000);
	uint32_t milliseconds = now_timestamp % (1000 * 1000);

	uart_printf("[%5lu.%06lu][\033[36mI\033[0m] ", seconds, milliseconds);

	va_list args;
	va_start(args, fmt);
	va_list args_copy;
	va_copy(args_copy, args);
	vformat(uart_log_putchar, NULL, fmt, args_copy);
	va_end(args);
	va_end(args_copy);

	return 0;
}

/**
 * @brief Dump a memory range as hexadecimal bytes and printable ASCII.
 * @param[in] start_addr First byte to inspect.
 * @param[in] count Number of bytes to display.
 *
 * Output is grouped in 16-byte rows.  The function performs raw reads and
 * therefore requires the caller to provide a valid, readable address range.
 */
void dump_hex(uintptr_t start_addr, uint32_t count)
{
	const uint8_t *ptr = (const uint8_t *)start_addr;
	const uint8_t *end = ptr + count;

	while (ptr < end) {
		const uint8_t *line = ptr;

		printk(LOG_LEVEL_MUTE, "%p: ", line);

		// Print hexadecimal bytes for each line
		for (int i = 0; i < 16; i++) {
			if (ptr < end) {
				printk(LOG_LEVEL_MUTE, "%02X ", *ptr);
				ptr++;
			} else {
				printk(LOG_LEVEL_MUTE, "   "); // Pad with spaces for incomplete bytes
			}
		}

		// Print corresponding printable ASCII characters for each line
		printk(LOG_LEVEL_MUTE, " ");
		for (int i = 0; i < 16; i++) {
			if (line + i < end) {
				char c = line[i];
				if (c >= 32 && c <= 126) {
					printk(LOG_LEVEL_MUTE, "%c", c); // Printable character
				} else {
					printk(LOG_LEVEL_MUTE, "."); // Replace non-printable character with dot
				}
			} else {
				break;
			}
		}

		printk(LOG_LEVEL_MUTE, "\n");
	}
}
