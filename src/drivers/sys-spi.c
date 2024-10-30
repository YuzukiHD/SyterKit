/* SPDX-License-Identifier: Apache-2.0 */

#define LOG_LEVEL_DEFAULT LOG_LEVEL_INFO

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <timer.h>

#include <log.h>

#include <sys-spi.h>

/* DMA Handler */

static int sunxi_spi_dma_config(sunxi_spi_t *spi) {
}

static int sunxi_spi_dma_init(sunxi_spi_t *spi) {
}

static void sunxi_spi_gpio_init(sunxi_spi_t *spi) {
    /* Config SPI pins */
    sunxi_gpio_init(spi->gpio.gpio_cs.pin, spi->gpio.gpio_cs.mux);
    sunxi_gpio_init(spi->gpio.gpio_sck.pin, spi->gpio.gpio_sck.mux);
    sunxi_gpio_init(spi->gpio.gpio_mosi.pin, spi->gpio.gpio_mosi.mux);
    sunxi_gpio_init(spi->gpio.gpio_miso.pin, spi->gpio.gpio_miso.mux);
    sunxi_gpio_init(spi->gpio.gpio_wp.pin, spi->gpio.gpio_wp.mux);
    sunxi_gpio_init(spi->gpio.gpio_hold.pin, spi->gpio.gpio_hold.mux);

    /* Floating by default */
    sunxi_gpio_set_pull(spi->gpio.gpio_wp.pin, GPIO_PULL_UP);
    sunxi_gpio_set_pull(spi->gpio.gpio_hold.pin, GPIO_PULL_UP);
}

int sunxi_spi_init(sunxi_spi_t *spi) {
    if (spi->dma_handle != NULL) {
        sunxi_spi_dma_init(spi);
    }

    return 0;
}
