/* SPDX-License-Identifier: GPL-2.0+ */
#define pr_fmt(fmt) "panel: " fmt

/*
 * Panel command sequence interpreter. A sequence is an array of device tree
 * cells, each command being "type len arg data..." (see
 * dt-bindings/display/sunxi-display.h):
 *   1 0 ms            delay
 *   2 level pin       GPIO output (pin = bank * 32 + n)
 *   4 n ms bytes...   DCS write: bytes[0] is the command
 *   5 n ms bytes...   generic DSI write
 */

#include <stdbool.h>
#include <stdint.h>
#include <types.h>

#include <timer.h>

#include <common.h>
#include <log.h>

#include <drivers/display/display.h>

#define CMD_DELAY   1U
#define CMD_GPIO    2U
#define CMD_DCS	    4U
#define CMD_GENERIC 5U

static uint32_t be32(uint32_t value)
{
	return __builtin_bswap32(value);
}

static void panel_gpio(const sunxi_panel_t *panel, uint32_t pin, uint32_t level)
{
	gpio_mux_t gpio;
	uint32_t bank = pin / 32U;

	if (!panel->gpio_base || bank < panel->gpio_bank_base) {
		pr_warn("GPIO %u has no controller\n", (unsigned int)pin);
		return;
	}
	gpio.base = panel->gpio_base;
	gpio.pin = GPIO_PIN(bank, pin % 32U);
	gpio.bank = (uint8_t)(bank - panel->gpio_bank_base);
	gpio.mux = GPIO_OUTPUT;
	sunxi_gpio_init(&gpio);
	sunxi_gpio_set_value(&gpio, level ? 1 : 0);
	pr_debug("gpio P%c%u = %u (read %d)\n", (int)('A' + bank), (unsigned int)(pin % 32U),
		(unsigned int)(level ? 1 : 0), sunxi_gpio_read(&gpio));
}

int sunxi_panel_run_seq(const sunxi_panel_t *panel, const sunxi_disp_seq_t *seq, struct sunxi_dsi *dsi)
{
	uint32_t i = 0;
	uint8_t buf[64];

	while (seq && seq->cells && i + 3U <= seq->count) {
		uint32_t type = be32(seq->cells[i]);
		uint32_t len = be32(seq->cells[i + 1]);
		uint32_t arg = be32(seq->cells[i + 2]);
		int ret = 0;

		i += 3U;
		switch (type) {
		case CMD_DELAY:
			mdelay(arg);
			continue;
		case CMD_GPIO:
			/* len = level, arg = pin; a missing pin must not stop the rest */
			panel_gpio(panel, arg, len);
			continue;
		case CMD_DCS:
		case CMD_GENERIC: {
			uint32_t k;

			if (len == 0U || len > sizeof(buf) || i + len > seq->count)
				return DRIVER_ERROR_INVALID;
			for (k = 0; k < len; k++)
				buf[k] = (uint8_t)be32(seq->cells[i + k]);
			i += len;
			if (!dsi) {
				pr_err("DSI command without a DSI host\n");
				return DRIVER_ERROR_INVALID;
			}
			if (type == CMD_DCS)
				ret = sunxi_dsi_dcs_write((sunxi_dsi_t *)dsi, buf[0], buf + 1, len - 1U);
			else
				ret = sunxi_dsi_generic_write((sunxi_dsi_t *)dsi, buf, len);
			pr_debug("cmd %u len %u [%02x %02x] delay %u -> %d\n", (unsigned int)type, (unsigned int)len,
				buf[0], len > 1 ? buf[1] : 0, (unsigned int)arg, ret);
			if (ret < 0) {
				pr_err("command %u failed: %d\n", (unsigned int)type, ret);
				return ret;
			}
			if (arg)
				mdelay(arg);
			continue;
		}
		default:
			pr_err("unsupported command type %u\n", (unsigned int)type);
			return DRIVER_ERROR_INVALID;
		}
	}
	return DRIVER_OK;
}
