/* SPDX-License-Identifier: Apache-2.0 */

#ifndef __CLI_UART_H__
#define __CLI_UART_H__

#ifdef __cplusplus
extern "C" { 
#endif // __cplusplus

int uart_putchar(int c);

int uart_puts(const char *s);

int uart_getchar(void);

char get_uart_input(void);

void uart_log_putchar(void *arg, char c);

int tstc();

extern int puts(const char *);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif//__CLI_UART_H__