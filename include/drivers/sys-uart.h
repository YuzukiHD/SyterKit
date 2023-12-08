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
} sunxi_serial_t;

void sunxi_serial_init(sunxi_serial_t *uart);

void sunxi_serial_putc(void *arg, char c);

int sunxi_serial_tstc(void *arg);

char sunxi_serial_getc(void *arg);

#endif
