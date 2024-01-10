#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>

#include <common.h>

#include <reg-ncat.h>
#include <sys-clk.h>

#include <mmu.h>

#include <sys-dram.h>
#include <sys-gpio.h>
#include <sys-i2c.h>
#include <sys-sdcard.h>
#include <sys-sid.h>
#include <sys-spi.h>
#include <sys-uart.h>

sunxi_serial_t uart_dbg = {
        .base = SUNXI_UART0_BASE,
        .id = 0,
        .gpio_tx = {GPIO_PIN(GPIO_PORTB, 9), GPIO_PERIPH_MUX2},
        .gpio_rx = {GPIO_PIN(GPIO_PORTB, 10), GPIO_PERIPH_MUX2},
};

sunxi_spi_t sunxi_spi0 = {
        .base = SUNXI_SPI0_BASE,
        .id = 0,
        .clk_rate = 75 * 1000 * 1000,
        .gpio_cs = {GPIO_PIN(GPIO_PORTC, 1), GPIO_PERIPH_MUX4},
        .gpio_sck = {GPIO_PIN(GPIO_PORTC, 0), GPIO_PERIPH_MUX4},
        .gpio_mosi = {GPIO_PIN(GPIO_PORTC, 2), GPIO_PERIPH_MUX4},
        .gpio_miso = {GPIO_PIN(GPIO_PORTC, 3), GPIO_PERIPH_MUX4},
        .gpio_wp = {GPIO_PIN(GPIO_PORTC, 4), GPIO_PERIPH_MUX4},
        .gpio_hold = {GPIO_PIN(GPIO_PORTC, 5), GPIO_PERIPH_MUX4},
};

sdhci_t sdhci0 = {
        .name = "sdhci0",
        .reg = (sdhci_reg_t *) SUNXI_SMHC0_BASE,
        .voltage = MMC_VDD_27_36,
        .width = MMC_BUS_WIDTH_4,
        .clock = MMC_CLK_50M,
        .removable = 0,
        .isspi = FALSE,
        .gpio_clk = {GPIO_PIN(GPIO_PORTF, 2), GPIO_PERIPH_MUX2},
        .gpio_cmd = {GPIO_PIN(GPIO_PORTF, 3), GPIO_PERIPH_MUX2},
        .gpio_d0 = {GPIO_PIN(GPIO_PORTF, 1), GPIO_PERIPH_MUX2},
        .gpio_d1 = {GPIO_PIN(GPIO_PORTF, 0), GPIO_PERIPH_MUX2},
        .gpio_d2 = {GPIO_PIN(GPIO_PORTF, 5), GPIO_PERIPH_MUX2},
        .gpio_d3 = {GPIO_PIN(GPIO_PORTF, 4), GPIO_PERIPH_MUX2},
};

sunxi_i2c_t i2c_pmu = {
        .base = SUNXI_RTWI_BASE,
        .id = SUNXI_R_I2C0,
        .speed = 4000000,
        .gpio_scl = {GPIO_PIN(GPIO_PORTL, 0), GPIO_PERIPH_MUX3},
        .gpio_sda = {GPIO_PIN(GPIO_PORTL, 1), GPIO_PERIPH_MUX3},
};