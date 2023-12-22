/* SPDX-License-Identifier: Apache-2.0 */

#ifndef __SYS_SPI_H__
#define __SYS_SPI_H__

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include "sys-clk.h"
#include "sys-gpio.h"

#include "log.h"

typedef enum {
    SPI_IO_SINGLE = 0x00,
    SPI_IO_DUAL_RX,
    SPI_IO_QUAD_RX,
    SPI_IO_QUAD_IO,
} spi_io_mode_t;

typedef struct {
    uint32_t base;
    uint8_t id;
    uint32_t clk_rate;
    gpio_mux_t gpio_cs;
    gpio_mux_t gpio_sck;
    gpio_mux_t gpio_miso;
    gpio_mux_t gpio_mosi;
    gpio_mux_t gpio_wp;
    gpio_mux_t gpio_hold;
} sunxi_spi_t;

int sunxi_spi_init(sunxi_spi_t *spi);

void sunxi_spi_disable(sunxi_spi_t *spi);

int sunxi_spi_transfer(sunxi_spi_t *spi, spi_io_mode_t mode, void *txbuf, uint32_t txlen, void *rxbuf, uint32_t rxlen);

#endif
