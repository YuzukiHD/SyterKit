/* SPDX-License-Identifier: Apache-2.0 */

#ifndef __SUNXI_USART_H__
#define __SUNXI_USART_H__

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include "sys-clk.h"
#include "sys-gpio.h"
#include "sys-uart.h"

#include "log.h"

typedef struct {
    uint32_t base;
    uint8_t id;
    gpio_mux_t gpio_tx;
    gpio_mux_t gpio_rx;
} sunxi_uart_t;

void sunxi_uart_init(sunxi_uart_t *uart);

void sunxi_uart_putc(void *arg, char c);

#endif
