/* SPDX-License-Identifier: Apache-2.0 */

#ifndef __SYS_GPIO_V2_H__
#define __SYS_GPIO_V2_H__

#include <inttypes.h>
#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include "log.h"

enum {
    GPIO_INPUT = 0,
    GPIO_OUTPUT = 1,
    GPIO_PERIPH_MUX2 = 2,
    GPIO_PERIPH_MUX3 = 3,
    GPIO_PERIPH_MUX4 = 4,
    GPIO_PERIPH_MUX5 = 5,
    GPIO_PERIPH_MUX6 = 6,
    GPIO_PERIPH_MUX7 = 7,
    GPIO_PERIPH_MUX8 = 8,
    GPIO_PERIPH_MUX14 = 14,
    GPIO_DISABLED = 0xf,
};

enum {
    GPIO_PORTA = 0,
    GPIO_PORTB,
    GPIO_PORTC,
    GPIO_PORTD,
    GPIO_PORTE,
    GPIO_PORTF,
    GPIO_PORTG,
    GPIO_PORTH,
    GPIO_PORTI,
    GPIO_PORTJ,
    GPIO_PORTK,
    GPIO_PORTL,
};

enum gpio_pull_t {
    GPIO_PULL_UP = 0,
    GPIO_PULL_DOWN = 1,
    GPIO_PULL_NONE = 2,
};

typedef uint32_t gpio_t;
#define PIO_NUM_IO_BITS 5

#define GPIO_PIN(x, y) (((uint32_t) (x << PIO_NUM_IO_BITS)) | y)

typedef struct {
    gpio_t pin;
    uint8_t mux;
} gpio_mux_t;

extern void sunxi_gpio_init(gpio_t pin, int cfg);

extern void sunxi_gpio_set_value(gpio_t pin, int value);

extern int sunxi_gpio_read(gpio_t pin);

extern void sunxi_gpio_set_pull(gpio_t pin, enum gpio_pull_t pull);

#endif // __SYS_GPIO_V2_H__