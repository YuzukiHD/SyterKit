/* SPDX-License-Identifier: GPL-2.0+ */
#define pr_fmt(fmt) "backlight: " fmt

#include <stdbool.h>
#include <stdint.h>
#include <types.h>

#include <common.h>
#include <log.h>

#include <drivers/display/display.h>

int sunxi_backlight_set(sunxi_backlight_t *bl, bool on)
{
	if (!bl || bl->type == SUNXI_BACKLIGHT_NONE)
		return DRIVER_OK;

	if (bl->type == SUNXI_BACKLIGHT_GPIO) {
		sunxi_gpio_init(&bl->gpio);
		sunxi_gpio_set_value(&bl->gpio, (on != 0) == (bl->gpio_active_high != 0));
		return DRIVER_OK;
	}

	if (!bl->pwm.status)
		sunxi_pwm_init(&bl->pwm);

	if (!on) {
		sunxi_pwm_release(&bl->pwm, bl->pwm_channel);
		return DRIVER_OK;
	}

	sunxi_pwm_config_t cfg = {
		.period_ns = bl->pwm_period_ns,
		.duty_ns = (uint32_t)((uint64_t)bl->pwm_period_ns * bl->level / 100U),
		.polarity = bl->pwm_active_high ? PWM_POLARITY_NORMAL : PWM_POLARITY_INVERSED,
		.pwm_mode = PWM_MODE_CYCLE,
	};

	int ret = sunxi_pwm_set_config(&bl->pwm, bl->pwm_channel, &cfg);

	pr_debug("pwm channel %u period %u duty %u -> %d\n", bl->pwm_channel, (unsigned int)cfg.period_ns,
		(unsigned int)cfg.duty_ns, ret);
	return ret;
}
