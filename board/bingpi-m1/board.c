#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>

#include <common.h>

#include <sys-clk.h>
#include <reg-ncat.h>

#include <mmu.h>

#include <sys-gpio.h>
#include <sys-spi.h>
#include <sys-uart.h>
#include <sys-sdcard.h>

sunxi_serial_t uart_dbg = {
        .base = SUNXI_UART0_BASE,
        .id = 0,
        .gpio_tx = {GPIO_PIN(GPIO_PORTH, 9), GPIO_PERIPH_MUX5},
        .gpio_rx = {GPIO_PIN(GPIO_PORTH, 10), GPIO_PERIPH_MUX5},
};

void clean_syterkit_data(void) {
    /* Disable MMU, data cache, instruction cache, interrupts */
    arm32_mmu_disable();
    printk(LOG_LEVEL_INFO, "disable mmu ok...\n");
    arm32_dcache_disable();
    printk(LOG_LEVEL_INFO, "disable dcache ok...\n");
    arm32_icache_disable();
    printk(LOG_LEVEL_INFO, "disable icache ok...\n");
    arm32_interrupt_disable();
    printk(LOG_LEVEL_INFO, "free interrupt ok...\n");
}

void sunxi_serial_init_v3s(sunxi_serial_t *uart) {
    uint32_t addr;
    uint32_t val;

    /* Open the clock gate for uart */
    addr = CCU_BASE + CCU_BUS_CLK_GATE3;
    val = read32(addr);
    val |= 1 << uart->id;
    write32(addr, val);

    /* Deassert USART reset */
    addr = CCU_BASE + CCU_BUS_SOFT_RST4;
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