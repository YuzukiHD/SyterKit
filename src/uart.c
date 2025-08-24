/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <timer.h>

#include <sys-uart.h>

#include <uart.h>

extern sunxi_serial_t uart_dbg;

/* Transmit a character over the UART */
void uart_log_putchar(void *arg, char c) {
	if (c == '\n') {
		/* If the character is a newline, transmit a carriage return before newline */
		sunxi_serial_putc(&uart_dbg, '\r');
	}
	/* Transmit the character */
	sunxi_serial_putc(&uart_dbg, c);
}

/* Transmit a character over the UART */
int uart_putchar(int c) {
	if (c == '\n') {
		/* If the character is a newline, transmit a carriage return before newline */
		sunxi_serial_putc(&uart_dbg, '\r');
	}
	/* Transmit the character */
	sunxi_serial_putc(&uart_dbg, c);
	/* Return success */
	return 0;
}

/* Transmit a string over the UART */
int uart_puts(const char *s) {
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

/* Receive a character from the UART */
int uart_getchar(void) {
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

/* Retrieve input character from the UART receiver buffer */
char get_uart_input(void) {
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

/* Check if there are characters available in the input buffer */
int tstc() {
	return sunxi_serial_tstc(&uart_dbg);
}

/* Clib Porting */
int puts(const char *s) {
	return uart_puts(s);
}
