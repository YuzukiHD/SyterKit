/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __SYS_GPIO_H__
#define __SYS_GPIO_H__

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include "log.h"

#ifdef __cplusplus
extern "C" {
#endif// __cplusplus

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
	GPIO_LEVEL_LOW = 0,
	GPIO_LEVEL_HIGH,
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
	GPIO_PORTM,
	GPIO_PORTN,
};

enum gpio_pull_t {
	GPIO_PULL_UP = 0,
	GPIO_PULL_DOWN = 1,
	GPIO_PULL_NONE = 2,
};

typedef uint32_t gpio_drv_t;
typedef uint32_t gpio_t;

#define PIO_NUM_IO_BITS 5

#define GPIO_PIN(x, y) (((uint32_t) (x << PIO_NUM_IO_BITS)) | y)

typedef struct {
	gpio_t pin;
	uint8_t mux;
} gpio_mux_t;

/**
 * Initialize the specified GPIO pin with the given configuration.
 *
 * @param pin The GPIO pin to initialize.
 * @param cfg The configuration value for the GPIO pin.
 */
void sunxi_gpio_init(gpio_t pin, int cfg);

/**
 * Set the value of the specified GPIO pin.
 *
 * @param pin The GPIO pin to set the value for.
 * @param value The value to be set (0 or 1) for the GPIO pin.
 */
void sunxi_gpio_set_value(gpio_t pin, int value);

/**
 * Read the value of the specified GPIO pin.
 *
 * @param pin The GPIO pin to read the value from.
 * @return The value (0 or 1) read from the GPIO pin.
 */
int sunxi_gpio_read(gpio_t pin);

/**
 * Set the pull configuration for the specified GPIO pin.
 *
 * @param pin The GPIO pin to set the pull configuration for.
 * @param pull The pull configuration to be set for the GPIO pin.
 */
void sunxi_gpio_set_pull(gpio_t pin, enum gpio_pull_t pull);

/**
 * @brief Sets the drive strength of a Sunxi GPIO pin.
 *
 * This function sets the drive strength for the specified GPIO pin.
 *
 * @param pin The GPIO pin to set the drive strength for.
 * @param drv The drive strength value to set (GPIO_DRV_LOW, GPIO_DRV_MEDIUM, or GPIO_DRV_HIGH).
 */
void sunxi_gpio_set_drv(gpio_t pin, gpio_drv_t drv);

#ifdef __cplusplus
}
#endif// __cplusplus

#endif// __SYS_GPIO_H__