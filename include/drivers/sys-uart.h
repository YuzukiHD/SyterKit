/* SPDX-License-Identifier: Apache-2.0 */

#ifndef __SUNXI_USART_H__
#define __SUNXI_USART_H__

#include <stdarg.h>
#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <types.h>
#include <io.h>

#include "sys-gpio.h"
#include "sys-clk.h"
#include "sys-uart.h"

#include "log.h"

typedef struct {
	uint32_t   base;
	uint8_t	   id;
	gpio_mux_t gpio_tx;
	gpio_mux_t gpio_rx;
} sunxi_uart_t;

void sunxi_uart_init(sunxi_uart_t *uart);

void sunxi_uart_putc(void *arg, char c);

#endif
