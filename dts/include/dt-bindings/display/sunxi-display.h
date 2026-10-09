/* SPDX-License-Identifier: GPL-2.0+ */
#ifndef _DT_BINDINGS_DISPLAY_SUNXI_DISPLAY_H
#define _DT_BINDINGS_DISPLAY_SUNXI_DISPLAY_H

#include <dt-bindings/gpio/sunxi-gpio.h>

/* Global sunxi pin number, e.g. SUNXI_PIN(SUNXI_GPIO_PORT_D, 22) */
#define SUNXI_PIN(bank, n)		((bank) * 32 + (n))

/* allwinner,sunxi-rgb: allwinner,hv-mode */
#define SUNXI_HV_PARALLEL_RGB		0x0
#define SUNXI_HV_SERIAL_RGB		0x8
#define SUNXI_HV_SERIAL_RGB_DUMMY	0xa
#define SUNXI_HV_SERIAL_YUV		0xc
#define SUNXI_HV_CCIR656_2CYC		0xe

/* allwinner,sunxi-panel: allwinner,dsi-mode-flags */
#define SUNXI_DSI_MODE_VIDEO		(1 << 0)
#define SUNXI_DSI_MODE_VIDEO_BURST	(1 << 1)
#define SUNXI_DSI_MODE_VIDEO_SYNC_PULSE	(1 << 2)
#define SUNXI_DSI_MODE_LPM		(1 << 3)
#define SUNXI_DSI_CLOCK_NON_CONTINUOUS	(1 << 4)
#define SUNXI_DSI_MODE_NO_EOT_PACKET	(1 << 5)

/*
 * Command sequences (power-on/off, init, exit) are cell arrays, each
 * command being "type len arg data...".
 */
/* wait @ms milliseconds */
#define SUNXI_CMD_DELAY(ms)		1 0 ms
/* drive a GPIO to the physical @level (0/1) */
#define SUNXI_CMD_GPIO(pin, level)	2 level pin
/* DCS write: @n bytes (command byte and parameters), then @ms delay */
#define SUNXI_CMD_DCS(ms, n, bytes)	4 n ms bytes
/* generic MIPI DSI write */
#define SUNXI_CMD_GENERIC(ms, n, bytes)	5 n ms bytes

#endif
