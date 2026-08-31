/* SPDX-License-Identifier: GPL-2.0+ */
#define pr_fmt(fmt) "GPIO: " fmt

#include <io.h>
#include <stdint.h>

#include <log.h>
#include <drivers/gpio/gpio.h>

/* Sun55iw3 main-PIO power mode registers. */
#define GPIO_POW_MODE_SEL_REG 0x380U
#define GPIO_POW_MODE_CTL_REG 0x384U
#define GPIO_POW_MODE_VAL_REG 0x388U

/* Sun55 maps the shared B/H VCCIO detector to these bits. */
#define GPIO_POW_VCCIO_SEL_BIT 12U
#define GPIO_POW_VCCIO_CTL_BIT 12U
#define GPIO_POW_VCCIO_VAL_BIT 16U

/**
 * @brief Resolve power-mode register bits for a GPIO port
 * @details Determines the bit positions in the Sun55iw3 power mode selector,
 *          control, and value registers that correspond to the given GPIO port.
 *          The shared B/H VCCIO detector uses fixed bits, ordinary banks use
 *          their own bank index, and R_PIO ports are rejected.
 * @param gpio GPIO pin descriptor whose port is being resolved
 * @param sel_bit Output: bit in the mode-select register for this port
 * @param ctl_bit Output: bit in the mode-control register for this port
 * @param val_bit Output: bit in the mode-value register for this port
 * @return 0 on success, -1 if gpio is invalid or the port has no power mode
 */
static int sunxi_gpio_v2_pow_power_bits(const gpio_mux_t *gpio, uint32_t *sel_bit, uint32_t *ctl_bit, uint32_t *val_bit)
{
	uint32_t port;

	if (gpio == NULL || gpio->base == 0U || sel_bit == NULL || ctl_bit == NULL || val_bit == NULL)
		return -1;

	/* R_PIO has a different register block and no Sun55 main-PIO power mode. */
	port = gpio->pin >> PIO_NUM_IO_BITS;
	if (port >= GPIO_PORTL)
		return -1;

	if (port == GPIO_PORTB || port == GPIO_PORTH) {
		*sel_bit = GPIO_POW_VCCIO_SEL_BIT;
		*ctl_bit = GPIO_POW_VCCIO_CTL_BIT;
		*val_bit = GPIO_POW_VCCIO_VAL_BIT;
	} else if (port <= GPIO_PORTK) {
		/* Ordinary banks, including the eMMC PC bank, use their bank bit. */
		*sel_bit = port;
		*ctl_bit = port;
		*val_bit = port;
	} else {
		return -1;
	}

	return 0;
}

/**
 * @brief Read the I/O voltage level of a GPIO port
 * @details Resolves the port's bit in the Sun55iw3 power mode value register and
 *          reads it to determine whether the port is operating at 1.8 V or 3.3 V.
 * @param gpio GPIO pin descriptor whose port voltage is queried
 * @return GPIO_IO_VOLTAGE_1V8 or GPIO_IO_VOLTAGE_3V3 on success, -1 on error
 */
int sunxi_gpio_get_io_voltage(const gpio_mux_t *gpio)
{
	uint32_t val_bit;
	uint32_t unused_sel;
	uint32_t unused_ctl;

	if (sunxi_gpio_v2_pow_power_bits(gpio, &unused_sel, &unused_ctl, &val_bit) != 0)
		return -1;

	return (read32(gpio->base + GPIO_POW_MODE_VAL_REG) & BIT(val_bit)) != 0U ? (int)GPIO_IO_VOLTAGE_1V8 :
										   (int)GPIO_IO_VOLTAGE_3V3;
}

/**
 * @brief Set the I/O voltage level of a GPIO port
 * @details Validates the requested voltage, programs the withstand-voltage
 *          selector register (Sun55iw3 selector polarity is inverted: 0 selects
 *          1.8 V), then sets the control bit to disable self-adaptation for the
 *          port.
 * @param gpio GPIO pin descriptor whose port voltage is configured
 * @param voltage_uv Requested I/O voltage in microvolts (1.8 V or 3.3 V)
 * @return 0 on success, -1 on invalid voltage or unsupported port
 */
int sunxi_gpio_set_io_voltage(const gpio_mux_t *gpio, uint32_t voltage_uv)
{
	uint32_t sel_bit;
	uint32_t ctl_bit;
	uint32_t unused_val;
	uint32_t sel;
	uint32_t ctl;
	uint32_t mode;

	if (voltage_uv != GPIO_IO_VOLTAGE_1V8 && voltage_uv != GPIO_IO_VOLTAGE_3V3)
		return -1;
	if (sunxi_gpio_v2_pow_power_bits(gpio, &sel_bit, &ctl_bit, &unused_val) != 0)
		return -1;

	/* Sun55iw3 selector polarity is reversed: 0 selects 1.8 V. */
	mode = voltage_uv == GPIO_IO_VOLTAGE_1V8 ? 0U : 1U;

	/* Program the withstand-voltage selector, then disable self-adaptation. */
	sel = read32(gpio->base + GPIO_POW_MODE_SEL_REG);
	sel &= ~BIT(sel_bit);
	sel |= mode << sel_bit;
	write32(gpio->base + GPIO_POW_MODE_SEL_REG, sel);

	ctl = read32(gpio->base + GPIO_POW_MODE_CTL_REG);
	ctl |= BIT(ctl_bit);
	write32(gpio->base + GPIO_POW_MODE_CTL_REG, ctl);

	pr_debug("bank %u: voltage=%u mV, POW_VAL=0x%08x, MODE_SEL=0x%08x, MODE_CTL=0x%08x\n",
		gpio->pin >> PIO_NUM_IO_BITS, voltage_uv / 1000U, read32(gpio->base + GPIO_POW_MODE_VAL_REG),
		read32(gpio->base + GPIO_POW_MODE_SEL_REG), read32(gpio->base + GPIO_POW_MODE_CTL_REG));
	return 0;
}
