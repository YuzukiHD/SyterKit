/* SPDX-License-Identifier: GPL-2.0+ */

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

/**
 * @brief SPI Input/Output Mode Enumeration
 * 
 * This enum defines the different SPI I/O modes.
 */
typedef enum {
	SPI_IO_SINGLE = 0x00, /**< Single I/O mode, using one data line */
	SPI_IO_DUAL_RX,		  /**< Dual I/O mode, using two data lines for receiving */
	SPI_IO_QUAD_RX,		  /**< Quad I/O mode, using four data lines for receiving */
	SPI_IO_QUAD_IO,		  /**< Quad I/O mode, using four data lines for both transmitting and receiving */
} spi_io_mode_t;

/**
 * @brief SPI Speed Mode Enumeration
 * 
 * This enum defines the different SPI clock frequencies.
 */
typedef enum {
	SPI_LOW_FREQUENCY = 24000000,  /**< Low frequency: 24 MHz */
	SPI_MOD_FREQUENCY = 50000000,  /**< Medium frequency: 50 MHz */
	SPI_HIGH_FREQUENCY = 60000000, /**< High frequency: 60 MHz */
	SPI_MAX_FREQUENCY = 100000000, /**< Maximum frequency: 100 MHz */
} spi_speed_mode_t;

/**
 * @brief SPI Clock CDR Mode Enumeration
 * 
 * This enum defines the clock CDR (Clock Data Recovery) modes.
 */
typedef enum {
	SPI_CDR1_MODE = 0, /**< Clock Data Recovery mode 1 */
	SPI_CDR2_MODE = 1, /**< Clock Data Recovery mode 2 */
	SPI_CDR_NONE = 2,  /**< No Clock Data Recovery mode */
} spi_clk_cdr_mode_t;

/**
 * @brief SPI GPIO Configuration Structure
 * 
 * This struct holds the GPIO configuration for the SPI interface.
 */
typedef struct {
	gpio_mux_t gpio_cs;	  /**< Chip Select GPIO pin */
	gpio_mux_t gpio_sck;  /**< SPI Clock (SCK) GPIO pin */
	gpio_mux_t gpio_miso; /**< Master In Slave Out (MISO) GPIO pin */
	gpio_mux_t gpio_mosi; /**< Master Out Slave In (MOSI) GPIO pin */
	gpio_mux_t gpio_wp;	  /**< Write Protect GPIO pin */
	gpio_mux_t gpio_hold; /**< Hold GPIO pin */
} sunxi_spi_gpio_t;

/**
 * @brief SPI Clock Configuration Structure
 * 
 * This struct holds the configuration for the SPI clock.
 */
typedef struct {
	uint32_t spi_clock_cfg_base;		/**< Base address of the SPI clock configuration */
	uint32_t spi_clock_source;			/**< Source of the SPI clock */
	uint32_t spi_clock_factor_n_offset; /**< Clock factor offset */
	uint32_t spi_clock_freq;			/**< SPI clock frequency */
	spi_clk_cdr_mode_t cdr_mode;		/**< Clock mode */
} sunxi_spi_clk_t;

/**
 * @brief SPI Device Configuration Structure
 * 
 * This struct holds the configuration for an SPI device, including its clock and GPIO settings.
 */
typedef struct {
	uint32_t base;				/**< Base address of the SPI peripheral */
	uint8_t id;					/**< SPI device ID */
	uint32_t clk_rate;			/**< Clock rate for the SPI device */
	sunxi_spi_gpio_t gpio;		/**< GPIO configuration for the SPI device */
	sunxi_dma_t *dma_handle;	/**< DMA handle for the SPI device */
	sunxi_clk_t parent_clk_reg; /**< Parent clock register configuration */
	sunxi_spi_clk_t spi_clk;	/**< SPI clock configuration */
} sunxi_spi_t;

#define MAX_FIFU (64)						   /**< Maximum FIFO size set to 64. */
#define SPI_CLK_SEL_PERIPH_300M (0x1)		   /**< Selects the SPI peripheral clock to 300 MHz. */
#define SPI_CLK_SEL_PERIPH_200M (0x2)		   /**< Selects the SPI peripheral clock to 200 MHz. */
#define SPI_CLK_SEL_FACTOR_N_OFF (8)		   /**< Offset for the SPI clock select factor is 8. */
#define SPI_DEFAULT_CLK_RST_OFFSET(x) (x + 16) /**< Returns the default clock reset offset, based on the SPI module number (x). */
#define SPI_DEFAULT_CLK_GATE_OFFSET(x) (x)	   /**< Returns the default clock gate offset, based on the SPI module number (x). */

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
