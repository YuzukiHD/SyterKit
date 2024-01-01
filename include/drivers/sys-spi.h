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

#ifdef __cplusplus
extern "C" {
#endif// __cplusplus

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

/**
 * Initialize the Sunxi SPI controller with the specified configuration.
 *
 * @param spi Pointer to the Sunxi SPI controller structure.
 * @return 0 if successful, or an error code if failed.
 */
int sunxi_spi_init(sunxi_spi_t *spi);

/**
 * Disable the Sunxi SPI controller.
 *
 * @param spi Pointer to the Sunxi SPI controller structure.
 */
void sunxi_spi_disable(sunxi_spi_t *spi);

/**
 * Perform a SPI transfer using the Sunxi SPI controller.
 *
 * @param spi Pointer to the Sunxi SPI controller structure.
 * @param mode The SPI IO mode to use for the transfer (e.g., SPI_IO_MODE_SINGLE, SPI_IO_MODE_DUAL, SPI_IO_MODE_QUAD).
 * @param txbuf Pointer to the buffer containing the data to transmit.
 * @param txlen The length of the data to transmit in bytes.
 * @param rxbuf Pointer to the buffer to store the received data.
 * @param rxlen The length of the data to receive in bytes.
 * @return 0 if successful, or an error code if failed.
 */
int sunxi_spi_transfer(sunxi_spi_t *spi, spi_io_mode_t mode, void *txbuf, uint32_t txlen, void *rxbuf, uint32_t rxlen);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif
