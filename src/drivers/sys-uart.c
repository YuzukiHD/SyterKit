/**
 * @file sys-uart.c
 * @brief Allwinner Platform UART (Universal Asynchronous Receiver/Transmitter) Driver
 *
 * This file implements the UART driver for Allwinner platforms. The UART driver provides
 * functionality for serial communication, including initialization, configuration,
 * and basic input/output operations. It supports various UART settings such as baud rate,
 * parity, stop bits, and data length.
 *
 * SPDX-License-Identifier: GPL-2.0+ */

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

/**
 * @brief Initialize the UART clock
 * 
 * This function configures the UART clock by setting the reset control and enabling
 * the clock gate for the UART module. It prepares the UART hardware for further configuration.
 *
 * @param[in] uart Pointer to the UART structure containing clock configuration
 */
void sunxi_serial_clock_init(sunxi_serial_t *uart) {
	sunxi_clk_t uart_clk = uart->uart_clk;
	/* Set CLK RST */
	setbits_le32(uart_clk.rst_reg_base, BIT(uart_clk.rst_reg_offset));
	/* Open Gate */
	clrbits_le32(uart_clk.gate_reg_base, BIT(uart_clk.gate_reg_offset));
	udelay(10);
	setbits_le32(uart_clk.gate_reg_base, BIT(uart_clk.gate_reg_offset));
}

/**
 * @brief Initialize the UART interface
 * 
 * This function initializes the UART interface by configuring the clock, baud rate,
 * line control settings (parity, stop bits, data length), FIFO control, and GPIO pins.
 * It sets up the UART for serial communication based on the provided configuration.
 *
 * @param[in] uart Pointer to the UART structure containing complete configuration
 *                 including base address, clock settings, baud rate, and GPIO pins
 */
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

/**
 * @brief Output a character to the UART
 * 
 * This function sends a single character to the UART transmit buffer. It waits until
 * the transmit holding register is empty before writing the character.
 *
 * @param[in] arg Pointer to the UART structure (cast to void* for compatibility)
 * @param[in] c The character to be transmitted
 */
void __attribute__((weak)) sunxi_serial_putc(void *arg, char c) {
	sunxi_serial_t *uart = (sunxi_serial_t *) arg;
	sunxi_serial_reg_t *serial_reg = (sunxi_serial_reg_t *) uart->base;

	while ((serial_reg->lsr & (1 << 6)) == 0)
		;
	serial_reg->thr = c;
}

/**
 * @brief Read a character from the UART
 * 
 * This function reads a single character from the UART receive buffer. It waits until
 * data is available before reading.
 *
 * @param[in] arg Pointer to the UART structure (cast to void* for compatibility)
 * @return The received character
 */
char __attribute__((weak)) sunxi_serial_getc(void *arg) {
	sunxi_serial_t *uart = (sunxi_serial_t *) arg;
	sunxi_serial_reg_t *serial_reg = (sunxi_serial_reg_t *) uart->base;

	while ((serial_reg->lsr & 1) == 0)
		;
	return serial_reg->rbr;
}

/**
 * @brief Check if a character is available to read from the UART
 * 
 * This function checks the UART line status register to determine if there is
 * data available in the receive buffer.
 *
 * @param[in] arg Pointer to the UART structure (cast to void* for compatibility)
 * @return Non-zero value if data is available, zero otherwise
 */
int __attribute__((weak)) sunxi_serial_tstc(void *arg) {
	sunxi_serial_t *uart = (sunxi_serial_t *) arg;
	sunxi_serial_reg_t *serial_reg = (sunxi_serial_reg_t *) uart->base;

	return serial_reg->lsr & 1;
}