/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __CLI_UART_H__
#define __CLI_UART_H__

#ifdef __cplusplus
extern "C" {
#endif// __cplusplus

/**
 * @brief Writes a single character 'c' to the UART output.
 *
 * @param[in] c The character to write.
 * @return Zero after the character has been written.
 */
int uart_putchar(int c);

/**
 * @brief Writes the null-terminated string 's' to the UART output.
 *
 * @param[in] s The string to write.
 * @return One after the complete string has been written.
 */
int uart_puts(const char *s);

/**
 * @brief Reads a single character from the UART input.
 *
 * @return The received character, with carriage return converted to newline.
 */
int uart_getchar(void);

/**
 * @brief Waits for and returns a single character from the UART input.
 *
 * @return The received character, or zero if the receive timeout expires.
 */
char get_uart_input(void);

/**
 * @brief Writes a single character 'c' to the log output.
 *
 * @param[in] arg Ignored callback context.
 * @param[in] c The character to write.
 */
void uart_log_putchar(void *arg, char c);

/**
 * @brief Tests whether a character is waiting in the UART input buffer.
 *
 * @return 1 if a character is waiting, 0 otherwise.
 */
int tstc(void);

/**
 * @brief Writes the null-terminated string 's' to the standard output.
 *
 * @param[in] s The string to write.
 * @return One after the complete string has been written.
 */
extern int puts(const char *s);

#ifdef __cplusplus
}
#endif// __cplusplus

#endif//__CLI_UART_H__
