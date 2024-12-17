/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __CLI_UART_H__
#define __CLI_UART_H__

#ifdef __cplusplus
extern "C" {
#endif// __cplusplus

/**
 * Writes a single character 'c' to the UART output.
 *
 * @param c The character to be written.
 * @return The character that was written.
 */
int uart_putchar(int c);

/**
 * Writes the null-terminated string 's' to the UART output.
 *
 * @param s The string to be written.
 * @return The number of characters written, excluding the terminating null byte.
 */
int uart_puts(const char *s);

/**
 * Reads a single character from the UART input.
 *
 * @return The character that was read.
 */
int uart_getchar(void);

/**
 * Waits for and returns a single character from the UART input.
 *
 * @return The character that was read.
 */
char get_uart_input(void);

/**
 * Writes a single character 'c' to the log output.
 *
 * @param arg A pointer to optional arguments.
 * @param c The character to be written.
 */
void uart_log_putchar(void *arg, char c);

/**
 * Tests whether a character is waiting in the UART input buffer.
 *
 * @return 1 if a character is waiting, 0 otherwise.
 */
int tstc();

/**
 * Writes the null-terminated string 's' to the standard output.
 *
 * @param s The string to be written.
 * @return The number of characters written, excluding the terminating null byte.
 */
extern int puts(const char *s);

#ifdef __cplusplus
}
#endif// __cplusplus

#endif//__CLI_UART_H__