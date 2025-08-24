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

typedef struct uart_serial {
	volatile uint32_t rbr; /* 0 */
	volatile uint32_t ier; /* 1 */
	volatile uint32_t fcr; /* 2 */
	volatile uint32_t lcr; /* 3 */
	volatile uint32_t mcr; /* 4 */
	volatile uint32_t lsr; /* 5 */
	volatile uint32_t msr; /* 6 */
	volatile uint32_t sch; /* 7 */
} uart_serial_t;

#define thr rbr
#define dll rbr
#define dlh ier
#define iir fcr

#define SUNXI_UART0_BASE 0x05000000

static uart_serial_t *uart_dbg;

static uint32_t init_timestamp = 0;

void set_timer_count() {
	init_timestamp = read32(SUNXI_RTC_DATA_BASE + RTC_FEL_INDEX * 4);
}

void sunxi_serial_init() {
	uart_dbg = (uart_serial_t *) SUNXI_UART0_BASE;
}

// Function to transmit a single character via UART
void sunxi_uart_putc(char c) {
	while ((uart_dbg->lsr & (1 << 6)) == 0)
		;
	uart_dbg->thr = c;
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
