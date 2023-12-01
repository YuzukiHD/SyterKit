/* SPDX-License-Identifier: Apache-2.0 */

#include <stdarg.h>
#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <types.h>
#include <io.h>

#include <log.h>

#include <sys-clk.h>

#include "sys-uart.h"

void sunxi_uart_init(sunxi_uart_t *uart)
{
	uint32_t addr;
	uint32_t val;

	/* Config uart TXD and RXD pins */
	sunxi_gpio_init(uart->gpio_tx.pin, uart->gpio_tx.mux);
	sunxi_gpio_init(uart->gpio_rx.pin, uart->gpio_rx.mux);

	/* Open the clock gate for uart */
	addr = CCU_BASE + CCU_UART_BGR_REG;
	val	 = read32(addr);
	val |= 1 << uart->id;
	write32(addr, val);

	/* Deassert USART reset */
	addr = CCU_BASE + CCU_UART_BGR_REG;
	val	 = read32(addr);
	val |= 1 << (16 + uart->id);
	write32(addr, val);

	/* Config USART to 115200-8-1-0 */
	addr = uart->base;
	write32(addr + 0x04, 0x0);
	write32(addr + 0x08, 0xf7);
	write32(addr + 0x10, 0x0);
	val = read32(addr + 0x0c);
	val |= (1 << 7);
	write32(addr + 0x0c, val);
	write32(addr + 0x00, 0xd & 0xff);
	write32(addr + 0x04, (0xd >> 8) & 0xff);
	val = read32(addr + 0x0c);
	val &= ~(1 << 7);
	write32(addr + 0x0c, val);
	val = read32(addr + 0x0c);
	val &= ~0x1f;
	val |= (0x3 << 0) | (0 << 2) | (0x0 << 3);
	write32(addr + 0x0c, val);
}

void sunxi_uart_putc(void *arg, char c)
{
	sunxi_uart_t *uart = (sunxi_uart_t *)arg;

	while ((read32(uart->base + 0x7c) & (0x1 << 1)) == 0)
		;
	write32(uart->base + 0x00, c);
}
