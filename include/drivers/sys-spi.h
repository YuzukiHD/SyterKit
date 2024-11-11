/* SPDX-License-Identifier: Apache-2.0 */

#ifndef __SYS_SPI_H__
#define __SYS_SPI_H__

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <sys-clk.h>
#include <sys-dma.h>
#include <sys-gpio.h>

#include <log.h>

#include <reg-spi.h>

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
    gpio_mux_t gpio_cs;
    gpio_mux_t gpio_sck;
    gpio_mux_t gpio_miso;
    gpio_mux_t gpio_mosi;
    gpio_mux_t gpio_wp;
    gpio_mux_t gpio_hold;
} sunxi_spi_gpio_t;

typedef struct {
    uint32_t spi_clock_cfg_base;
    uint32_t spi_clock_source;
    uint32_t spi_clock_factor_n_offset;
    uint32_t spi_clock_freq;
} sunxi_spi_clk_t;

typedef struct {
    uint32_t base;
    uint8_t id;
    uint32_t clk_rate;
    sunxi_spi_gpio_t gpio;
    sunxi_dma_t *dma_handle;
    sunxi_clk_t parent_clk_reg;
    sunxi_spi_clk_t spi_clk;
} sunxi_spi_t;

#define MAX_FIFU (64)

#define SPI_CLK_SEL_PERIPH_300M (0x1)
#define SPI_CLK_SEL_PERIPH_200M (0x2)
#define SPI_CLK_SEL_FACTOR_N_OFF (8)

#define SPI_DEFAULT_CLK_RST_OFFSET(x) (x + 16)
#define SPI_DEFAULT_CLK_GATE_OFFSET(x) (x)

/**
 * @brief Initializes the SPI interface.
 * 
 * This function initializes the SPI interface by configuring the GPIO pins, clock, bus, and counters.
 * If a DMA handle is set, DMA mode is used for data transfers. The function calls several other SPI
 * initialization functions to set up the SPI hardware.
 * 
 * @param spi Pointer to the SPI structure containing configuration and register information.
 * 
 * @return 0 on success.
 */
int sunxi_spi_init(sunxi_spi_t *spi);

/**
 * @brief Disables the SPI interface.
 * 
 * This function disables the SPI bus, deinitializes DMA (if used), and deinitializes the SPI clock. 
 * It is called when the SPI interface is no longer needed or when performing cleanup operations.
 * 
 * @param spi Pointer to the SPI structure containing configuration and register information.
 */
void sunxi_spi_disable(sunxi_spi_t *spi);

/**
 * @brief Updates the SPI clock rate.
 * 
 * This function updates the SPI clock rate and reinitializes the clock, bus, and transfer control settings 
 * to apply the new clock rate.
 * 
 * @param spi Pointer to the SPI structure containing configuration and register information.
 * @param clk The new clock rate to be set.
 * 
 * @return 0 on success.
 */
int sunxi_spi_update_clk(sunxi_spi_t *spi, uint32_t clk);

/**
 * @brief Performs SPI data transfer.
 * 
 * This function initiates a data transfer on the SPI bus. The transfer can be either full-duplex (both 
 * transmission and reception) or half-duplex (only transmission or reception). The transfer is done based 
 * on the specified SPI I/O mode. The function handles both transmit and receive operations, including the 
 * use of DMA if required for large transfers.
 * 
 * @param spi Pointer to the SPI structure containing configuration and register information.
 * @param mode The I/O mode to use for the transfer (e.g., single, dual, quad).
 * @param txbuf Pointer to the transmission buffer.
 * @param txlen Length of the transmission data in bytes.
 * @param rxbuf Pointer to the reception buffer.
 * @param rxlen Length of the reception data in bytes.
 * 
 * @return The total number of bytes transferred (txlen + rxlen).
 */
int sunxi_spi_transfer(sunxi_spi_t *spi, spi_io_mode_t mode, void *txbuf, uint32_t txlen, void *rxbuf, uint32_t rxlen);

#ifdef __cplusplus
}
#endif// __cplusplus

#endif
