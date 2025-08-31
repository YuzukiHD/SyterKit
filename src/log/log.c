/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <timer.h>
#include <types.h>

#include "log.h"
#include "uart.h"
#include "xformat.h"

void printk(int level, const char *fmt, ...) {
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
			uart_printf("[%5lu.%06lu][\033[30mT\033[37m] ", seconds, milliseconds);
			break;
		case LOG_LEVEL_DEBUG:
			uart_printf("[%5lu.%06lu][\033[32mD\033[37m] ", seconds, milliseconds);
			break;
		case LOG_LEVEL_INFO:
			uart_printf("[%5lu.%06lu][\033[36mI\033[37m] ", seconds, milliseconds);
			break;
		case LOG_LEVEL_WARNING:
			uart_printf("[%5lu.%06lu][\033[33mW\033[37m] ", seconds, milliseconds);
			break;
		case LOG_LEVEL_ERROR:
			uart_printf("[%5lu.%06lu][\033[31mE\033[37m] ", seconds, milliseconds);
			break;
		case LOG_LEVEL_BACKTRACE:
			uart_printf("[%5lu.%06lu][\033[38;5;214mB\033[37m] ", seconds, milliseconds);
		case LOG_LEVEL_MUTE:
		default:
			break;
	}
#endif
	va_list args;
	va_start(args, fmt);
	va_list args_copy;
	va_copy(args_copy, args);
	xvformat(uart_log_putchar, NULL, fmt, args_copy);
	va_end(args);
	va_end(args_copy);
}

void uart_printf(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	va_list args_copy;
	va_copy(args_copy, args);
	xvformat(uart_log_putchar, NULL, fmt, args_copy);
	va_end(args);
	va_end(args_copy);
}

int printf(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	va_list args_copy;
	va_copy(args_copy, args);
	xvformat(uart_log_putchar, NULL, fmt, args_copy);
	va_end(args);
	va_end(args_copy);

	return 0;
}

int printf_dram(const char *fmt, ...) {
	uint32_t now_timestamp = time_us() - get_init_timestamp();
	uint32_t seconds = now_timestamp / (1000 * 1000);
	uint32_t milliseconds = now_timestamp % (1000 * 1000);

	uart_printf("[%5lu.%06lu][\033[36mI\033[37m] ", seconds, milliseconds);

	va_list args;
	va_start(args, fmt);
	va_list args_copy;
	va_copy(args_copy, args);
	xvformat(uart_log_putchar, NULL, fmt, args_copy);
	va_end(args);
	va_end(args_copy);

	return 0;
}

void dump_hex(uint32_t start_addr, uint32_t count) {
	uint8_t *ptr = (uint8_t *) start_addr;
	uint32_t end_addr = start_addr + count;

	while (ptr < (uint8_t *) end_addr) {
		printk(LOG_LEVEL_MUTE, "%08X: ", (uint32_t) ptr);

		// Print hexadecimal bytes for each line
		for (int i = 0; i < 16; i++) {
			if (ptr < (uint8_t *) end_addr) {
				printk(LOG_LEVEL_MUTE, "%02X ", *ptr);
				ptr++;
			} else {
				printk(LOG_LEVEL_MUTE, "   ");// Pad with spaces for incomplete bytes
			}
		}

		// Print corresponding printable ASCII characters for each line
		ptr -= 16;
		printk(LOG_LEVEL_MUTE, " ");
		for (int i = 0; i < 16; i++) {
			if (ptr < (uint8_t *) end_addr) {
				char c = *ptr;
				if (c >= 32 && c <= 126) {
					printk(LOG_LEVEL_MUTE, "%c", c);// Printable character
				} else {
					printk(LOG_LEVEL_MUTE, ".");// Replace non-printable character with dot
				}
				ptr++;
			} else {
				break;
			}
		}

		printk(LOG_LEVEL_MUTE, "\n");
	}
}