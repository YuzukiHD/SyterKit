#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <io.h>

#include <log.h>
#include <sys-uart.h>

#include <sys-clk.h>

void sunxi_serial_init(sunxi_serial_t *uart) {
    uint32_t addr;
    uint32_t val;

    /* Open the clock gate for uart */
    addr = CCU_BASE + CCU_UART_BGR_REG;
    val = read32(addr);
    val |= 1 << uart->id;
    write32(addr, val);

    /* Deassert USART reset */
    addr = CCU_BASE + CCU_UART_BGR_REG;
    val = read32(addr);
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

    /* Config uart TXD and RXD pins */
    sunxi_gpio_init(uart->gpio_tx.pin, uart->gpio_tx.mux);
    sunxi_gpio_init(uart->gpio_rx.pin, uart->gpio_rx.mux);
}

void sunxi_serial_putc(void *arg, char c) {
    sunxi_serial_t *uart = (sunxi_serial_t *) arg;

    while ((read32(uart->base + 0x14) & (0x1 << 6)) == 0)
        ;
    write32(uart->base + 0x00, c);
}

char sunxi_serial_getc(void *arg) {
    sunxi_serial_t *uart = (sunxi_serial_t *) arg;

    while ((read32(uart->base + 0x14) & 1) == 0)
        ;
    return read32(uart->base + 0x00);
}

int sunxi_serial_tstc(void *arg) {
    sunxi_serial_t *uart = (sunxi_serial_t *) arg;

    return (read32(uart->base + 0x14)) & 1;
}