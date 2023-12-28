#include <byteorder.h>
#include <endian.h>
#include <io.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <types.h>

#include <rtc.h>
#include <xformat.h>

#define SUNXI_UART0_BASE 0x05000000

static uint32_t init_timestamp = 0;

void set_timer_count() {
    init_timestamp = read32(SUNXI_RTC_DATA_BASE + RTC_FEL_INDEX * 4);
}

// Function to transmit a single character via UART
void sunxi_uart_putc(char c) {
    // Wait until the TX FIFO is not full
    while ((read32(SUNXI_UART0_BASE + 0x14) & (0x1 << 6)) == 0)
        ;

    // Write the character to the UART data register
    write32(SUNXI_UART0_BASE + 0x00, c);
}

void uart_log_putchar(void *arg, char c) {
    if (c == '\n') {
        /* If the character is a newline, transmit a carriage return before newline */
        sunxi_uart_putc('\r');
    }
    /* Transmit the character */
    sunxi_uart_putc(c);
}

// Output a formatted string to the standard output
void uart_printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    va_list args_copy;
    va_copy(args_copy, args);
    xvformat(uart_log_putchar, NULL, fmt, args_copy);
    va_end(args);
    va_end(args_copy);
}

void printf(const char *fmt, ...) {
    uint32_t now_timestamp = time_ms() - init_timestamp;
    uint32_t seconds = now_timestamp / 1000;
    uint32_t milliseconds = now_timestamp % 1000;
    uart_printf("[%5lu.%06lu][I] ", seconds, milliseconds);
    va_list args;
    va_start(args, fmt);
    va_list args_copy;
    va_copy(args_copy, args);
    xvformat(uart_log_putchar, NULL, fmt, args_copy);
    va_end(args);
    va_end(args_copy);
}
