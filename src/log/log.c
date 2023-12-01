/* SPDX-License-Identifier: MIT */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <types.h>

#include "common.h"
#include "log.h"
#include "xformat.h"

extern sunxi_uart_t uart_dbg;

static uint32_t init_timestamp = 0;

void set_timer_count()
{
    init_timestamp = time_ms();
}

void printk(int level, const char* fmt, ...)
{
	if (level < LOG_LEVEL_DEFAULT) {
        return;
    }
	uint32_t now_timestamp = time_ms() - init_timestamp;
    uint32_t seconds = now_timestamp / 1000;
    uint32_t milliseconds = now_timestamp % 1000;

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
        case LOG_LEVEL_MUTE:  
        default:
            break;
    }
    va_list args;
    va_start(args, fmt);
    va_list args_copy;
    va_copy(args_copy, args);
    xvformat(sunxi_uart_putc, &uart_dbg, fmt, args_copy);
    va_end(args);
    va_end(args_copy);
}

void uart_printf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    va_list args_copy;
    va_copy(args_copy, args);
    xvformat(sunxi_uart_putc, &uart_dbg, fmt, args_copy);
    va_end(args);
    va_end(args_copy);
}
