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

#define CCU_UART_RST_OFFSET (16)

static void sunxi_serial_clock_init(sunxi_serial_t *uart) {
    uint32_t addr;
    uint32_t val;

    /* Open the clock gate for uart */
    addr = CCU_BASE + CCU_UART_BGR_REG;
    val = read32(addr);
    val &= ~(1 << uart->id);
    write32(addr, val);
    udelay(10);
    val |= 1 << uart->id;
    write32(addr, val);

    /* Deassert USART reset */
    addr = CCU_BASE + CCU_UART_BGR_REG;
    val = read32(addr);
    val &= ~(1 << (CCU_UART_RST_OFFSET + uart->id));
    write32(addr, val);
    udelay(10);
    val |= 1 << (CCU_UART_RST_OFFSET + uart->id);
    write32(addr, val);
}

void sunxi_serial_init(sunxi_serial_t *uart) {
    uint32_t addr;
    uint32_t val;

    sunxi_serial_clock_init(uart);

    /* set default to 115200-8-1-0 for backwords compatibility */
    if (uart->baud_rate == 0) {
        uart->baud_rate = UART_BAUDRATE_115200;
        uart->dlen = UART_DLEN_8;
        uart->stop = UART_STOP_BIT_0;
        uart->parity = UART_PARITY_NO;
    }

    sunxi_serial_reg_t *serial_reg = (sunxi_serial_reg_t *) uart->base;
    serial_reg->mcr = 0x3;
    uint32_t uart_clk = (24000000 + 8 * uart->baud_rate) / (16 * uart->baud_rate);
    serial_reg->lcr |= 0x80;
    serial_reg->dlh = uart_clk >> 8;
    serial_reg->dll = uart_clk & 0xff;
    serial_reg->lcr &= ~0x80;
    serial_reg->lcr = ((uart->parity & 0x03) << 3) | ((uart->stop & 0x01) << 2) | (uart->dlen & 0x03);
    serial_reg->fcr = 0x7;

    /* Config uart TXD and RXD pins */
    sunxi_gpio_init(uart->gpio_tx.pin, uart->gpio_tx.mux);
    sunxi_gpio_init(uart->gpio_rx.pin, uart->gpio_rx.mux);
}

void sunxi_serial_putc(void *arg, char c) {
    sunxi_serial_t *uart = (sunxi_serial_t *) arg;
    sunxi_serial_reg_t *serial_reg = (sunxi_serial_reg_t *) uart->base;

    while ((serial_reg->lsr & (1 << 6)) == 0)
        ;
    serial_reg->thr = c;
}

char sunxi_serial_getc(void *arg) {
    sunxi_serial_t *uart = (sunxi_serial_t *) arg;
    sunxi_serial_reg_t *serial_reg = (sunxi_serial_reg_t *) uart->base;

	while ((serial_reg->lsr & 1) == 0)
		;
	return serial_reg->rbr;
}

int sunxi_serial_tstc(void *arg) {
    sunxi_serial_t *uart = (sunxi_serial_t *) arg;
    sunxi_serial_reg_t *serial_reg = (sunxi_serial_reg_t *) uart->base;

    return serial_reg->lsr & 1;
}