/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file uart.c
 * @brief UART console adapters used by logging and the command shell.
 *
 * Early output is either buffered until the device-tree-selected UART is
 * ready or discarded, depending on the build configuration.  Once the
 * console is marked ready, all helpers delegate to the serial driver.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <timer.h>

#include <drivers/serial/serial.h>

#include <uart.h>

sunxi_serial_t uart_dbg;

#ifdef CONFIG_UART_EARLY_LOG
#define UART_EARLY_LOG_BUFFER_SIZE CONFIG_UART_EARLY_LOG_BUFFER_SIZE

static char uart_early_log_buffer[UART_EARLY_LOG_BUFFER_SIZE];
static size_t uart_early_log_length;
#endif
static bool uart_log_console_is_ready;

/**
 * @brief Send one character directly to the initialized UART hardware.
 * @param[in] c Character to transmit.
 *
 * The UART driver expects an explicit carriage return before a newline, so
 * this low-level adapter expands line feeds for terminal compatibility.
 */
static void uart_log_putchar_hw(char c)
{
	if (c == '\n')
		sunxi_serial_putc(&uart_dbg, '\r');
	sunxi_serial_putc(&uart_dbg, c);
}

#ifdef CONFIG_UART_EARLY_LOG
/**
 * @brief Flush characters captured before the UART console was ready.
 *
 * The early-log buffer is emitted in insertion order and then reset so a
 * second console transition cannot replay old messages.
 */
static void uart_log_flush_early(void)
{
	size_t i;

	for (i = 0; i < uart_early_log_length; i++)
		uart_log_putchar_hw(uart_early_log_buffer[i]);
	uart_early_log_length = 0;
}
#endif

/**
 * @brief Emit one character through the early-safe logging callback.
 * @param[in] arg Unused callback context; accepted for formatter compatibility.
 * @param[in] c Character to emit.
 *
 * Before console initialization this function buffers output when early
 * logging is enabled.  After initialization it writes directly to hardware.
 */
void uart_log_putchar(void *arg, char c)
{
	(void)arg;
#ifdef CONFIG_UART_EARLY_LOG
	if (!uart_log_console_is_ready) {
		if (uart_early_log_length < UART_EARLY_LOG_BUFFER_SIZE)
			uart_early_log_buffer[uart_early_log_length++] = c;
		return;
	}
#else
	if (!uart_log_console_is_ready)
		return;
#endif
	uart_log_putchar_hw(c);
}

/**
 * @brief Mark the UART console ready and replay buffered early output.
 *
 * Calling this function more than once is harmless.  The first call changes
 * the console state and, when configured, flushes all buffered characters.
 */
void uart_log_console_ready(void)
{
	if (uart_log_console_is_ready)
		return;
	uart_log_console_is_ready = true;
#ifdef CONFIG_UART_EARLY_LOG
	/* Emit all pre-console output immediately after UART initialization. */
	uart_log_flush_early();
#endif
}

/**
 * @brief Write one character to the logging UART.
 * @param[in] c Character value to transmit.
 * @return Always zero, matching the firmware's historical console API.
 */
int uart_putchar(int c)
{
	uart_log_putchar(NULL, (char)c);
	/* Return success */
	return 0;
}

/**
 * @brief Write a NUL-terminated string to the logging UART.
 * @param[in] s String to transmit; it must not be NULL.
 * @return One after all characters have been submitted.
 */
int uart_puts(const char *s)
{
	const char *c = s;
	/* Iterate through the characters in the string */
	while (*c != '\0') {
		/* Transmit each character */
		uart_putchar(*c);
		c++;
	}
	/* Return success */
	return 1;
}

/**
 * @brief Read one character and normalize carriage return to newline.
 * @return Received character, or zero when the input polling timeout expires.
 */
int uart_getchar(void)
{
	/* Get input character from the UART */
	int c = get_uart_input();
	if (c == '\r') {
		/* If the character is a carriage return, return newline character instead */
		return '\n';
	} else {
		/* Otherwise, return the character as-is */
		return c;
	}
}

/**
 * @brief Poll the UART receiver for a bounded period.
 * @return Received character, or zero when no character arrives within 10 ms.
 */
char get_uart_input(void)
{
	char c = 0;
	/* Get the current time in milliseconds */
	uint32_t start_time = time_ms();
	/* Loop until a character is received or more than 10 milliseconds have passed */
	while (1) {
		if (sunxi_serial_tstc(&uart_dbg)) {
			/* If there is data available in the UART receiver buffer, read the received character */
			c = sunxi_serial_getc(&uart_dbg);
			/* Exit the loop */
			break;
		}
		if (time_ms() - start_time > 10) {
			/* If more than 10 milliseconds have passed, exit the loop */
			break;
		}
		/* Delay for 500 microseconds */
		udelay(500);
	}
	/* Return the received character */
	return c;
}

/**
 * @brief Test whether the debug UART has a character available.
 * @return Non-zero when a character can be read, otherwise zero.
 */
int tstc(void)
{
	return sunxi_serial_tstc(&uart_dbg);
}

/**
 * @brief Provide the freestanding `puts` symbol using the UART console.
 * @param[in] s String to transmit; it must be NUL-terminated.
 * @return The result returned by @ref uart_puts.
 */
int puts(const char *s)
{
	return uart_puts(s);
}
