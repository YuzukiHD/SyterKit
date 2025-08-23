/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <io.h>
#include <timer.h>

#include <log.h>
#include <sys-uart.h>

#include <sys-clk.h>

void sunxi_serial_clock_init(sunxi_serial_t *uart) {
	sunxi_clk_t uart_clk = uart->uart_clk;
	/* Set CLK RST */
	setbits_le32(uart_clk.rst_reg_base, BIT(uart_clk.rst_reg_offset));
	/* Open Gate */
	clrbits_le32(uart_clk.gate_reg_base, BIT(uart_clk.gate_reg_offset));
	udelay(10);
	setbits_le32(uart_clk.gate_reg_base, BIT(uart_clk.gate_reg_offset));
}

void sunxi_serial_init(sunxi_serial_t *uart) {
	sunxi_serial_clock_init(uart);

	/* Typecast to sunxi_serial_reg_t structure pointer */
	sunxi_serial_reg_t *serial_reg = (sunxi_serial_reg_t *) uart->base;

	/* Set control register MCR to 0x3 */
	serial_reg->mcr = 0x3;

	/* Calculate UART clock frequency */
	uint32_t uart_clk = (uart->uart_clk.parent_clk + 8 * uart->baud_rate) / (16 * uart->baud_rate);

	/* Set bit 7 of line control register LCR */
	serial_reg->lcr |= 0x80;

	/* Set divisor latch high register DLH */
	serial_reg->dlh = uart_clk >> 8;

	/* Set divisor latch low register DLL */
	serial_reg->dll = uart_clk & 0xff;

	/* Clear bit 7 of line control register LCR */
	serial_reg->lcr &= ~0x80;

	/* Set parity, stop bits, and data length in line control register LCR */
	/* Set parity bits based on uart->parity value */
	serial_reg->lcr |= (uart->parity & 0x03) << 3;

	/* Set stop bits based on uart->stop value */
	serial_reg->lcr |= (uart->stop & 0x01) << 2;

	/* Set data length based on uart->dlen value */
	serial_reg->lcr |= uart->dlen & 0x03;

	/* Configure FIFO Control Register (FCR) */
	/* Bit 0: FIFO Enable (1 - Enable FIFO) */
	/* Bit 1: RCVR Reset (1 - Clear Receive FIFO) */
	/* Bit 2: XMIT Reset (1 - Clear Transmit FIFO) */
	/* Bit 3: DMA Mode Select (0 - Disable DMA Mode) */
	/* Bit 4: Reserved (0) */
	/* Bits 5-7: Trigger Level (011 - Trigger Level of 8 bytes) */
	serial_reg->fcr = 0x7;

	/* Config uart TXD and RXD pins */
	sunxi_gpio_init(uart->gpio_pin.gpio_tx.pin, uart->gpio_pin.gpio_tx.mux);
	sunxi_gpio_init(uart->gpio_pin.gpio_rx.pin, uart->gpio_pin.gpio_rx.mux);
}

void __attribute__((weak)) sunxi_serial_putc(void *arg, char c) {
	sunxi_serial_t *uart = (sunxi_serial_t *) arg;
	sunxi_serial_reg_t *serial_reg = (sunxi_serial_reg_t *) uart->base;

	while ((serial_reg->lsr & (1 << 6)) == 0)
		;
	serial_reg->thr = c;
}

char __attribute__((weak)) sunxi_serial_getc(void *arg) {
	sunxi_serial_t *uart = (sunxi_serial_t *) arg;
	sunxi_serial_reg_t *serial_reg = (sunxi_serial_reg_t *) uart->base;

	while ((serial_reg->lsr & 1) == 0)
		;
	return serial_reg->rbr;
}

int __attribute__((weak)) sunxi_serial_tstc(void *arg) {
	sunxi_serial_t *uart = (sunxi_serial_t *) arg;
	sunxi_serial_reg_t *serial_reg = (sunxi_serial_reg_t *) uart->base;

	return serial_reg->lsr & 1;
}