/* SPDX-License-Identifier: GPL-2.0+ */

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>

#include <reg-ncat.h>
#include <sys-gpio.h>

enum {
	GPIO_CFG0 = 0x00,
	GPIO_CFG1 = 0x04,
	GPIO_CFG2 = 0x08,
	GPIO_CFG3 = 0x0c,
	GPIO_DAT = 0x10,
	GPIO_DRV0 = 0x14,
	GPIO_DRV1 = 0x18,
	GPIO_PUL0 = 0x1c,
	GPIO_PUL1 = 0x20,
	GPIO_OFFSET = 0x24,
	GPIO_CFG_MASK = 0x7,
	GPIO_DRV_MASK = 0x3,
};

/**
 * @brief Extracts the port number from a GPIO pin.
 *
 * This inline function extracts the port number from the given GPIO pin number.
 *
 * @param pin The GPIO pin number.
 * @return The port number.
 */
static inline uint32_t _port_num(gpio_t pin) {
	return pin >> PIO_NUM_IO_BITS;
}

/**
 * @brief Gets the base address of the port register for a GPIO pin.
 *
 * This function calculates and returns the base address of the port register
 * for the specified GPIO pin.
 *
 * @param pin The GPIO pin number.
 * @return The base address of the port register.
 */
static uint32_t _port_base_get(gpio_t pin) {
	uint32_t port = pin >> PIO_NUM_IO_BITS;

	/* PL PM PN in R_PIO */
	if (port >= GPIO_PORTL) {
		return SUNXI_RPIO_BASE + (port - GPIO_PORTL) * GPIO_OFFSET;
	}
	/* PA PB PC PD PE PF PG PH PI PJ PK in PIO */
	return SUNXI_PIO_BASE + port * GPIO_OFFSET;
}

/**
 * @brief Extracts the pin number within a port from a GPIO pin.
 *
 * This inline function extracts the pin number within a port from the given
 * GPIO pin number.
 *
 * @param pin The GPIO pin number.
 * @return The pin number within a port.
 */
static inline uint32_t _pin_num(gpio_t pin) {
	return (pin & ((1 << PIO_NUM_IO_BITS) - 1));
}

/**
 * @brief Initializes a Sunxi GPIO pin with the specified configuration.
 *
 * This function initializes a Sunxi GPIO pin with the specified configuration. It sets
 * the pin's configuration based on the provided parameters.
 *
 * @param pin The GPIO pin to initialize.
 * @param cfg The configuration value for the GPIO pin.
 */
void sunxi_gpio_init(gpio_t pin, int cfg) {
	uint32_t port_addr = _port_base_get(pin);
	uint32_t pin_num = _pin_num(pin);
	uint32_t addr;
	uint32_t val;

	addr = port_addr + GPIO_CFG0 + ((pin_num >> 3) << 2);
	val = read32(addr);
	val &= ~(0xf << ((pin_num & 0x7) << 2));
	val |= ((cfg & GPIO_CFG_MASK) << ((pin_num & 0x7) << 2));
	write32(addr, val);

	printk_trace("GPIO: MUX pin = %d, num in bank = %d, addr = 0x%08x, val = 0x%08x, set cfg = %d\n", pin, pin_num, addr, read32(addr), cfg);
}

/**
 * @brief Sets the value of a Sunxi GPIO pin.
 *
 * This function sets the value of a Sunxi GPIO pin to the specified value (0 or 1).
 *
 * @param pin The GPIO pin to set the value for.
 * @param value The value to set (0 or 1).
 */
void sunxi_gpio_set_value(gpio_t pin, int value) {
	uint32_t port_addr = _port_base_get(pin);
	uint32_t pin_num = _pin_num(pin);
	uint32_t val;

	val = read32(port_addr + GPIO_DAT);
	val &= ~(1 << pin_num);
	val |= (!!value) << pin_num;
	write32(port_addr + GPIO_DAT, val);
}

/**
 * @brief Reads the value of a Sunxi GPIO pin.
 *
 * This function reads the current value of a Sunxi GPIO pin and returns it.
 *
 * @param pin The GPIO pin to read the value from.
 * @return The value of the GPIO pin (0 or 1).
 */
int sunxi_gpio_read(gpio_t pin) {
	uint32_t port_addr = _port_base_get(pin);
	uint32_t pin_num = _pin_num(pin);
	uint32_t val;

	val = read32(port_addr + GPIO_DAT);
	return !!(val & (1 << pin_num));
}

/**
 * @brief Sets the pull configuration of a Sunxi GPIO pin.
 *
 * This function sets the pull-up, pull-down, or no pull configuration for the specified GPIO pin.
 *
 * @param pin The GPIO pin to set the pull configuration for.
 * @param pull The pull configuration to set (GPIO_PULL_UP, GPIO_PULL_DOWN, or GPIO_PULL_NONE).
 */
void sunxi_gpio_set_pull(gpio_t pin, enum gpio_pull_t pull) {
	uint32_t port_addr = _port_base_get(pin);
	uint32_t pin_num = _pin_num(pin);
	uint32_t addr;
	uint32_t val, v;

	switch (pull) {
		case GPIO_PULL_UP:
			v = 0x1;
			break;

		case GPIO_PULL_DOWN:
			v = 0x2;
			break;

		case GPIO_PULL_NONE:
			v = 0x0;
			break;

		default:
			v = 0x0;
			break;
	}

	addr = port_addr + GPIO_PUL0 + ((pin_num >> 4) << 2);
	val = read32(addr);
	val &= ~(0x3 << ((pin_num & 0xf) << 1));
	val |= (v << ((pin_num & 0xf) << 1));
	write32(addr, val);

	printk_trace("GPIO: PULL pin = %d, addr = 0x%08x, val = 0x%08x, set pull = %d\n", pin, addr, read32(addr), v);
}

/**
 * @brief Sets the drive strength of a Sunxi GPIO pin.
 *
 * This function sets the drive strength for the specified GPIO pin.
 *
 * @param pin The GPIO pin to set the drive strength for.
 * @param drv The drive strength value to set (GPIO_DRV_LOW, GPIO_DRV_MEDIUM, or GPIO_DRV_HIGH).
 */
void sunxi_gpio_set_drv(gpio_t pin, gpio_drv_t drv) {
	uint32_t port_addr = _port_base_get(pin);
	uint32_t pin_num = _pin_num(pin);
	uint32_t addr;
	uint32_t val;

	addr = port_addr + GPIO_DRV0 + ((pin_num >> 4) << 2);
	val = read32(addr);
	val &= ~(0x3 << ((pin_num & 0xf) << 1));
	val |= (drv << ((pin_num & 0xf) << 1));
	write32(addr, val);

	printk_trace("GPIO: DRV pin = %d, addr = 0x%08x, val = 0x%08x, set drv = %d\n", pin, addr, read32(addr), drv);
}
