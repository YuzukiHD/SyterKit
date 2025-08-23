/* SPDX-License-Identifier: GPL-2.0+ */

#if LOG_LEVEL_DEFAULT <= LOG_LEVEL_DEBUG
/* #define LOG_LEVEL_DEFAULT LOG_LEVEL_DEBUG */
#endif

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
/**
 * @brief DMA configuration structure for SPI RX (Receive)
 * 
 * This structure is used for configuring the Direct Memory Access (DMA) 
 * controller for SPI data reception. It is placed in the section ".data" 
 * of the memory.
 */
static __attribute__((section(".data"))) sunxi_dma_set_t spi_rx_dma;

/**
 * @brief DMA handler for SPI
 * 
 * This variable holds the DMA handler for SPI operations. It is used to
 * manage the DMA transfer during SPI communication. It is initialized to 0
 * by default.
 */
static uint32_t spi_dma_handler = 0;


/**
 * @brief Perform a software reset on the SPI controller
 * 
 * This function triggers a software reset on the SPI controller by setting
 * the appropriate bit in the control register.
 * 
 * @param[in] spi A pointer to the SPI structure, which contains the base address
 *                of the SPI controller's registers.
 * 
 * @note This function is typically used to reset the SPI controller to its initial state.
 */
static inline void sunxi_spi_soft_reset(sunxi_spi_t *spi) {
	sunxi_spi_reg_t *spi_reg = (sunxi_spi_reg_t *) spi->base;
	spi_reg->gc |= SPI_GC_SRST;///< Set the software reset bit
}

/**
 * @brief Enable the SPI bus
 * 
 * This function enables the SPI bus by setting the enable bit in the control register.
 * 
 * @param[in] spi A pointer to the SPI structure, which contains the base address
 *                of the SPI controller's registers.
 * 
 * @note Enabling the bus allows SPI communication to begin.
 */
static inline void sunxi_spi_enable_bus(sunxi_spi_t *spi) {
	sunxi_spi_reg_t *spi_reg = (sunxi_spi_reg_t *) spi->base;
	spi_reg->gc |= SPI_GC_EN;///< Set the SPI enable bit
}

/**
 * @brief Disable the SPI bus
 * 
 * This function disables the SPI bus by clearing the enable bit in the control register.
 * 
 * @param[in] spi A pointer to the SPI structure, which contains the base address
 *                of the SPI controller's registers.
 * 
 * @note Disabling the bus stops SPI communication and can be used for power saving
 *       or for configuring the controller before restarting it.
 */
static inline void sunxi_spi_disable_bus(sunxi_spi_t *spi) {
	sunxi_spi_reg_t *spi_reg = (sunxi_spi_reg_t *) spi->base;
	spi_reg->gc &= ~SPI_GC_EN;///< Clear the SPI enable bit
}

/**
 * @brief Set the chip select (CS) for the SPI transfer
 * 
 * This function sets the chip select (CS) line by writing the value of the `cs` parameter
 * to the relevant bits in the SPI control register. The CS line is used to select which 
 * peripheral is active for communication.
 * 
 * @param[in] spi A pointer to the SPI structure, which contains the base address
 *                of the SPI controller's registers.
 * @param[in] cs  The chip select value to be set. This is typically a 0 or 1, corresponding
 *                to a specific peripheral on the SPI bus.
 * 
 * @note The CS line is used to enable or disable communication with specific SPI peripherals.
 */
static inline void sunxi_spi_set_cs(sunxi_spi_t *spi, uint8_t cs) {
	sunxi_spi_reg_t *spi_reg = (sunxi_spi_reg_t *) spi->base;
	spi_reg->tc &= ~SPI_TC_SS_MASK;			/* SS-chip select, clear two bits */
	spi_reg->tc |= cs << SPI_TC_SS_BIT_POS; /* Set chip select */
}

/**
 * @brief Set the SPI controller to master mode
 * 
 * This function sets the SPI controller to master mode by modifying the relevant control bit.
 * In master mode, the SPI controller generates clock signals for communication with slaves.
 * 
 * @param[in] spi A pointer to the SPI structure, which contains the base address
 *                of the SPI controller's registers.
 * 
 * @note In master mode, the SPI controller will control the clock line and initiate data transfers.
 */
static inline void sunxi_spi_set_master(sunxi_spi_t *spi) {
	sunxi_spi_reg_t *spi_reg = (sunxi_spi_reg_t *) spi->base;
	spi_reg->gc |= SPI_GC_MODE;///< Set the master mode bit
}

/**
 * @brief Start the SPI data transfer
 * 
 * This function triggers the start of a data transfer by setting the relevant bit
 * in the transfer control register. It begins the exchange of data on the SPI bus.
 * 
 * @param[in] spi A pointer to the SPI structure, which contains the base address
 *                of the SPI controller's registers.
 * 
 * @note This function should be called after configuring the SPI bus and chip select.
 */
static inline void sunxi_spi_start_xfer(sunxi_spi_t *spi) {
	sunxi_spi_reg_t *spi_reg = (sunxi_spi_reg_t *) spi->base;
	spi_reg->tc |= SPI_TC_XCH;///< Set the transfer start bit
}

/**
 * @brief Enable the SPI transmit pause feature
 * 
 * This function enables the transmit pause feature for the SPI controller.
 * When enabled, it allows the SPI transmission to be paused, providing control 
 * over when to send data.
 * 
 * @param[in] spi A pointer to the SPI structure, which contains the base address
 *                of the SPI controller's registers.
 * 
 * @note The transmit pause feature can be used to temporarily halt data transmission.
 */
static inline void sunxi_spi_enable_transmit_pause(sunxi_spi_t *spi) {
	sunxi_spi_reg_t *spi_reg = (sunxi_spi_reg_t *) spi->base;
	spi_reg->gc |= SPI_GC_TP_EN;///< Enable transmit pause
}

/**
 * @brief Set the SPI chip select (SS) ownership
 * 
 * This function controls the ownership of the chip select (SS) line. When the SS 
 * ownership is set to "on", the SPI controller has control over the SS line; 
 * when set to "off", the external controller or logic can take control.
 * 
 * @param[in] spi A pointer to the SPI structure, which contains the base address
 *                of the SPI controller's registers.
 * @param[in] on_off A value indicating whether to enable or disable SS ownership:
 *                   - 1: Enable SS ownership
 *                   - 0: Disable SS ownership
 * 
 * @note The SS line is critical in determining which device on the SPI bus is selected 
 *       for communication.
 */
static inline void sunxi_spi_set_ss_owner(sunxi_spi_t *spi, uint32_t on_off) {
	sunxi_spi_reg_t *spi_reg = (sunxi_spi_reg_t *) spi->base;
	on_off &= 0x1;// Ensure the on_off value is either 0 or 1
	if (on_off)
		spi_reg->tc |= SPI_TC_SS_OWNER;///< Set SS ownership
	else
		spi_reg->tc &= ~SPI_TC_SS_OWNER;///< Clear SS ownership
}

/**
 * @brief Query the number of bytes in the SPI transmit FIFO
 * 
 * This function queries the SPI transmit FIFO to determine how many bytes are 
 * currently waiting to be transmitted.
 * 
 * @param[in] spi A pointer to the SPI structure, which contains the base address
 *                of the SPI controller's registers.
 * 
 * @return The number of bytes currently in the transmit FIFO.
 * 
 * @note This function is useful for monitoring the status of the transmit FIFO 
 *       and managing data flow.
 */
static inline uint32_t sunxi_spi_query_txfifo(sunxi_spi_t *spi) {
	sunxi_spi_reg_t *spi_reg = (sunxi_spi_reg_t *) spi->base;
	uint32_t reg_val = (SPI_FIFO_STA_TX_CNT & spi_reg->fifo_sta);
	reg_val >>= SPI_TXCNT_BIT_POS;///< Shift to get the TX FIFO count
	return reg_val;
}

/**
 * @brief Query the number of bytes in the SPI receive FIFO
 * 
 * This function queries the SPI receive FIFO to determine how many bytes are 
 * currently available to be read.
 * 
 * @param[in] spi A pointer to the SPI structure, which contains the base address
 *                of the SPI controller's registers.
 * 
 * @return The number of bytes currently in the receive FIFO.
 * 
 * @note This function is useful for checking if data is available in the receive FIFO.
 */
static inline uint32_t sunxi_spi_query_rxfifo(sunxi_spi_t *spi) {
	sunxi_spi_reg_t *spi_reg = (sunxi_spi_reg_t *) spi->base;
	uint32_t reg_val = (SPI_FIFO_STA_RX_CNT & spi_reg->fifo_sta);
	reg_val >>= SPI_RXCNT_BIT_POS;///< Shift to get the RX FIFO count
	return reg_val;
}

/**
 * @brief Disable SPI interrupts
 * 
 * This function disables specific interrupts for the SPI controller based on the 
 * provided interrupt bitmap. The interrupt mask is applied to the interrupt control register.
 * 
 * @param[in] spi A pointer to the SPI structure, which contains the base address
 *                of the SPI controller's registers.
 * @param[in] bitmap A mask representing the interrupts to be disabled. Each bit corresponds 
 *                   to a specific interrupt.
 * 
 * @note This function can be used to disable unwanted interrupts to avoid unnecessary 
 *       interrupt handling.
 */
static inline void sunxi_spi_disable_irq(sunxi_spi_t *spi, uint32_t bitmap) {
	sunxi_spi_reg_t *spi_reg = (sunxi_spi_reg_t *) spi->base;
	bitmap &= SPI_INTEN_MASK;	///< Mask with the interrupt enable mask
	spi_reg->int_ctl &= ~bitmap;///< Disable the specified interrupts
}

/**
 * @brief Clear SPI interrupt pending flags
 * 
 * This function clears the interrupt pending flags for the SPI controller by writing 
 * the relevant bits to the interrupt status register.
 * 
 * @param[in] spi A pointer to the SPI structure, which contains the base address
 *                of the SPI controller's registers.
 * @param[in] pending_bit A mask representing the pending interrupt to be cleared.
 * 
 * @note Clearing interrupt flags is necessary after interrupt handling to prevent 
 *       repeated interrupts for the same event.
 */
static inline void sunxi_spi_clr_irq_pending(sunxi_spi_t *spi, uint32_t pending_bit) {
	sunxi_spi_reg_t *spi_reg = (sunxi_spi_reg_t *) spi->base;
	pending_bit &= SPI_INT_STA_MASK;///< Mask with the interrupt status mask
	spi_reg->int_sta = pending_bit; ///< Clear the pending interrupt flag
}

/**
 * @brief Query the status of pending SPI interrupts
 * 
 * This function queries the interrupt status register to determine which SPI interrupts
 * are pending. The function returns a mask of the pending interrupts.
 * 
 * @param[in] spi A pointer to the SPI structure, which contains the base address
 *                of the SPI controller's registers.
 * 
 * @return A mask representing the pending interrupts. Each bit corresponds to a specific 
 *         interrupt status.
 * 
 * @note This function is useful for checking which interrupts need to be handled.
 */
static inline uint32_t sunxi_spi_query_irq_pending(sunxi_spi_t *spi) {
	sunxi_spi_reg_t *spi_reg = (sunxi_spi_reg_t *) spi->base;
	return (SPI_INT_STA_MASK & spi_reg->int_sta);///< Return the pending interrupt status
}

/**
 * @brief Set the SPI chip select (SS) level
 * 
 * This function controls the logic level of the chip select (SS) line. It sets 
 * the SS line to either high or low level, depending on the `high_low` parameter.
 * 
 * @param[in] spi A pointer to the SPI structure, which contains the base address
 *                of the SPI controller's registers.
 * @param[in] high_low A value indicating the level of the SS line:
 *                     - 1: Set SS line to high
 *                     - 0: Set SS line to low
 * 
 * @note The SS line is used to select the SPI slave device for communication. 
 *       The level of the SS line controls when the SPI communication starts.
 */
static inline void sunxi_spi_set_ss_level(sunxi_spi_t *spi, uint32_t high_low) {
	sunxi_spi_reg_t *spi_reg = (sunxi_spi_reg_t *) spi->base;
	high_low &= 0x1;///< Ensure high_low is either 0 or 1
	if (high_low)
		spi_reg->tc |= SPI_TC_SS_LEVEL;///< Set SS line to high
	else
		spi_reg->tc &= ~SPI_TC_SS_LEVEL;///< Set SS line to low
}

/**
 * @brief Disable DMA for SPI data transmission and reception
 * 
 * This function disables the Direct Memory Access (DMA) for both transmitting and 
 * receiving data through the SPI interface. Disabling DMA may be useful when switching 
 * to interrupt or polling modes of data transfer.
 * 
 * @param[in] spi A pointer to the SPI structure, which contains the base address
 *                of the SPI controller's registers.
 * 
 * @note This function disables both the TX and RX DMA requests by clearing the 
 *       corresponding bits in the FIFO control register.
 */
static inline void sunxi_spi_dma_disable(sunxi_spi_t *spi) {
	sunxi_spi_reg_t *spi_reg = (sunxi_spi_reg_t *) spi->base;
	spi_reg->fifo_ctl &= ~(SPI_FIFO_CTL_TX_DRQEN | SPI_FIFO_CTL_RX_DRQEN);///< Disable TX and RX DMA
}

/**
 * @brief Reset the SPI FIFO buffers
 * 
 * This function resets the SPI transmit and receive FIFO buffers. The FIFO reset
 * is done by writing specific bits to the FIFO control register. It also configures 
 * the trigger levels for both receive and transmit FIFOs.
 * 
 * @param[in] spi A pointer to the SPI structure, which contains the base address
 *                of the SPI controller's registers.
 * 
 * @note The reset process clears the FIFO contents and sets the FIFO trigger levels
 *       to 0x20 for both TX and RX FIFO.
 */
static void sunxi_spi_reset_fifo(sunxi_spi_t *spi) {
	sunxi_spi_reg_t *spi_reg = (sunxi_spi_reg_t *) spi->base;
	uint32_t reg_val = spi_reg->fifo_ctl;

	reg_val |= (SPI_FIFO_CTL_RX_RST | SPI_FIFO_CTL_TX_RST);///< Reset RX and TX FIFOs
	// Set the trigger level for RX/TX FIFO
	reg_val &= ~(SPI_FIFO_CTL_RX_LEVEL | SPI_FIFO_CTL_TX_LEVEL);///< Clear the level bits
	reg_val |= (0x20 << 16) | 0x20;								///< Set trigger level to 0x20 for both RX and TX
	spi_reg->fifo_ctl = reg_val;
}

/**
 * @brief Read data from the SPI receive FIFO
 * 
 * This function reads data from the SPI receive FIFO into a buffer. It waits until
 * there is sufficient data available in the FIFO and transfers it into the provided
 * buffer.
 * 
 * @param[in] spi A pointer to the SPI structure, which contains the base address
 *                of the SPI controller's registers.
 * @param[out] buf A pointer to the buffer where the received data will be stored.
 * @param[in] len The number of bytes to read from the receive FIFO.
 * 
 * @return The number of bytes remaining to be read. This can be used to determine if 
 *         the entire requested data has been transferred.
 * 
 * @note This function reads data from the SPI FIFO in blocks, based on the FIFO shift size.
 *       It ensures that enough data is available in the FIFO before proceeding with the read.
 */
static uint32_t sunxi_spi_read_rx_fifo(sunxi_spi_t *spi, uint8_t *buf, uint32_t len) {
	sunxi_spi_reg_t *spi_reg = (sunxi_spi_reg_t *) spi->base;
	while ((len -= SPI_FIFO_CTL_SHIFT % SPI_FIFO_CTL_SHIFT) == 0) {
		// Wait until the RX FIFO has enough data to transfer
		while (sunxi_spi_query_rxfifo(spi) < SPI_FIFO_CTL_SHIFT) {}
		*(buf += SPI_FIFO_CTL_SHIFT) = spi_reg->rxdata;///< Read data from RX FIFO
	}

	while (len-- > 0) {
		// Wait for at least 1 byte in the RX FIFO
		while (sunxi_spi_query_rxfifo(spi) < 1)
			;
		*buf++ = read8((virtual_addr_t) &spi_reg->rxdata);///< Read 1 byte from RX FIFO
	}
	return len;
}

/**
 * @brief Write data to the SPI transmit FIFO
 * 
 * This function writes data from a buffer to the SPI transmit FIFO. It waits until
 * there is space available in the FIFO before writing more data.
 * 
 * @param[in] spi A pointer to the SPI structure, which contains the base address
 *                of the SPI controller's registers.
 * @param[in] buf A pointer to the buffer containing the data to be transmitted.
 * @param[in] len The number of bytes to write to the transmit FIFO.
 * 
 * @note This function writes data to the TX FIFO in blocks, based on the FIFO shift size.
 *       It ensures that there is space in the FIFO before writing more data.
 */
static void sunxi_spi_write_tx_fifo(sunxi_spi_t *spi, uint8_t *buf, uint32_t len) {
	sunxi_spi_reg_t *spi_reg = (sunxi_spi_reg_t *) spi->base;
	uint32_t tx_len = len;
	uint8_t *tx_buf = buf;

	while ((len -= SPI_FIFO_CTL_SHIFT % SPI_FIFO_CTL_SHIFT) == 0) {
		// Wait until there is space in the TX FIFO
		while (sunxi_spi_query_txfifo(spi) > MAX_FIFU - SPI_FIFO_CTL_SHIFT) {
			udelay(10);///< Small delay to allow TX FIFO to have space
		}
		spi_reg->txdata = *(buf += SPI_FIFO_CTL_SHIFT);///< Write data to TX FIFO
	}

	while (len-- > 0) {
		// Wait for space in the TX FIFO before writing more data
		while (sunxi_spi_query_txfifo(spi) >= MAX_FIFU) {
			udelay(10);///< Small delay to allow TX FIFO to have space
		}
		write8((virtual_addr_t) &spi_reg->txdata, *buf++);///< Write 1 byte to TX FIFO
	}
}

/**
 * @brief Perform SPI data reception using DMA
 * 
 * This function initiates a DMA transfer to read data from the SPI receive FIFO into
 * a provided buffer. It enables the DMA receive request and starts the DMA transfer. 
 * The function waits until the DMA transfer is complete before returning.
 * 
 * @param[in] spi A pointer to the SPI structure, which contains the base address
 *                of the SPI controller's registers.
 * @param[out] buf A pointer to the buffer where received data will be stored.
 * @param[in] len The number of bytes to read from the SPI receive FIFO.
 * 
 * @note This function uses DMA for data transfer, so it requires the DMA controller
 *       to be set up and ready to handle the transfer. The DMA handler `spi_dma_handler`
 *       must be initialized properly before calling this function.
 * 
 * @warning If the DMA transfer fails, a warning message will be printed using
 *          `printk_warning`.
 */
static void sunxi_spi_read_by_dma(sunxi_spi_t *spi, uint8_t *buf, uint32_t len) {
	sunxi_spi_reg_t *spi_reg = (sunxi_spi_reg_t *) spi->base;
	uint8_t ret = 0x0;

	// Initialize the buffer to zero
	memset(buf, 0x0, len);

	// Enable the RX DMA request in the FIFO control register
	spi_reg->fifo_ctl |= SPI_FIFO_CTL_RX_DRQEN;

	// Start the DMA transfer
	ret = sunxi_dma_start(spi_dma_handler, (uint32_t) &spi_reg->rxdata, (uint32_t) buf, len);
	if (ret) {
		printk_warning("SPI: DMA transfer failed\n");
	}

	// Wait for the DMA transfer to complete
	while (sunxi_dma_querystatus(spi_dma_handler))
		;
}

/**
 * @brief Set the SPI clock frequency
 * 
 * This function calculates and sets the SPI clock frequency based on the desired 
 * SPI clock (`spi_clk`), the system's master clock (`mclk`), and the clock divider 
 * mode (`cdr2`). It configures the clock control register to achieve the requested 
 * clock frequency for the SPI interface.
 * 
 * @param[in] spi A pointer to the SPI structure, which contains the base address
 *                of the SPI controller's registers.
 * @param[in] spi_clk The desired SPI clock frequency in Hz.
 * @param[in] mclk The master clock frequency in Hz.
 * @param[in] cdr2 If set to 1, the clock divider mode will be set to CDR2 (double the frequency).
 * 
 * @return The actual frequency of the SPI clock set by the function, in Hz.
 * 
 * @note If the requested SPI clock is different from the current parent clock,
 *       the function will attempt to adjust the clock divider to achieve the 
 *       desired SPI clock frequency.
 * 
 * @note The function prints debug messages regarding the clock divider settings 
 *       and the resulting actual SPI clock frequency using `printk_debug`.
 */
static uint32_t sunxi_spi_set_clk(sunxi_spi_t *spi, uint32_t spi_clk, uint32_t mclk, uint32_t cdr_mode) {
	sunxi_spi_reg_t *spi_reg = (sunxi_spi_reg_t *) spi->base;

	uint32_t reg = 0;
	uint32_t div = 0;
	uint32_t src_clk = mclk;
	uint32_t freq = 0;

	/* CDR2 mode: use a clock divider that divides by 2 (if cdr2 is set) */
	if (cdr_mode == SPI_CDR2_MODE) {
		div = mclk / (spi_clk * 2) - 1;///< Calculate divider for CDR2 mode
		reg &= ~SPI_CLK_CTL_CDR2;
		reg |= (div | SPI_CLK_CTL_DRS);
		printk_debug("SPI: CDR2 - n = %lu\n", div);
		freq = mclk / (2 * (div + 1));		///< Calculate actual SPI frequency
	} else if (cdr_mode == SPI_CDR1_MODE) { /* CDR1 mode: divide the source clock by powers of 2 */
		while (src_clk > spi_clk) {
			div++;
			src_clk >>= 1;
		}
		reg &= ~(SPI_CLK_CTL_CDR1 | SPI_CLK_CTL_DRS);
		reg |= (div << 8);///< Set CDR1 mode with the calculated divider
		printk_debug("SPI: CDR1 - n = %lu\n", div);
		freq = src_clk;///< Calculate actual SPI frequency
	} else {		   ///< cdr_mode none
		freq = src_clk;
		goto clk_out;
	}

	// Set the clock control register
	spi_reg->clk_ctl = reg;

clk_out:
	// Print debug information about clock divider and actual SPI frequency
	printk_debug("SPI: clock div=%u \n", div);
	printk_debug("SPI: set clock asked=%dMHz actual=%dMHz mclk=%dMHz\n", spi_clk / 1000000, freq / 1000000, mclk / 1000000);

	return freq;
}

/**
 * @brief Initialize the SPI DMA for data transfer.
 * 
 * This function initializes the DMA controller for SPI data reception. It requests
 * a DMA channel, configures the DMA settings for the SPI receive path, and installs 
 * the DMA interrupt handler. The DMA is then enabled and the necessary settings 
 * are applied to start data transfer via DMA.
 * 
 * @param[in] spi A pointer to the SPI structure, containing the necessary information
 *                about the SPI controller and DMA settings.
 * 
 * @return 0 on success, -1 on failure if DMA channel request fails.
 * 
 * @note The DMA handler must be available and correctly initialized before calling this function.
 * 
 * @warning If the DMA channel cannot be requested, an error message is printed using `printk_error`.
 */
static int sunxi_spi_dma_init(sunxi_spi_t *spi) {
	// Initialize the DMA controller using the SPI handle.
	sunxi_dma_init(spi->dma_handle);

	// Request a DMA channel for normal transfer.
	spi_dma_handler = sunxi_dma_request(DMAC_DMATYPE_NORMAL);

	if (spi_dma_handler == 0) {
		printk_error("SPI: DMA channel request failed\n");
		return -1;
	}

	/* Configure SPI RX DMA transfer settings */
	spi_rx_dma.loop_mode = 0;				// No loop mode for DMA transfer.
	spi_rx_dma.wait_cyc = 0x8;				// Wait cycles set to 8.
	spi_rx_dma.data_block_size = 1 * 32 / 8;// Data block size is 32 bits (4 bytes).

	// Configure source (SPI0) settings for DMA.
	spi_rx_dma.channel_cfg.src_drq_type = DMAC_CFG_TYPE_SPI0;			  // Source is SPI0.
	spi_rx_dma.channel_cfg.src_addr_mode = DMAC_CFG_SRC_ADDR_TYPE_IO_MODE;// Source address is I/O mode.
	spi_rx_dma.channel_cfg.src_burst_length = DMAC_CFG_SRC_8_BURST;		  // 8-byte burst length for source.
	spi_rx_dma.channel_cfg.src_data_width = DMAC_CFG_SRC_DATA_WIDTH_32BIT;// Source data width is 32 bits.

	// Configure destination (DRAM) settings for DMA.
	spi_rx_dma.channel_cfg.dst_drq_type = DMAC_CFG_TYPE_DRAM;				   // Destination is DRAM.
	spi_rx_dma.channel_cfg.dst_addr_mode = DMAC_CFG_DEST_ADDR_TYPE_LINEAR_MODE;// Destination address is linear mode.
	spi_rx_dma.channel_cfg.dst_burst_length = DMAC_CFG_DEST_8_BURST;		   // 8-byte burst length for destination.
	spi_rx_dma.channel_cfg.dst_data_width = DMAC_CFG_DEST_DATA_WIDTH_32BIT;	   // Destination data width is 32 bits.

	// Install DMA interrupt handler and enable interrupts.
	sunxi_dma_install_int(spi_dma_handler, NULL);
	sunxi_dma_enable_int(spi_dma_handler);

	// Set DMA transfer settings.
	sunxi_dma_setting(spi_dma_handler, &spi_rx_dma);

	return 0;// Success
}

/**
 * @brief Deinitialize the SPI DMA.
 * 
 * This function disables the DMA interrupt for the SPI DMA channel, effectively 
 * deinitializing the DMA setup and preparing for cleanup.
 * 
 * @param[in] spi A pointer to the SPI structure.
 * 
 * @return 0 on success.
 * 
 * @note This function is typically called when SPI DMA operations are no longer required.
 */
static int sunxi_spi_dma_deinit(sunxi_spi_t *spi) {
	// Disable DMA interrupts for the current SPI DMA channel.
	sunxi_dma_disable_int(spi_dma_handler);

	return 0;// Success
}

/**
 * @brief Get the current SPI clock frequency.
 * 
 * This function calculates and returns the current SPI clock frequency based 
 * on the SPI clock configuration registers. It reads the configuration, 
 * extracts the relevant clock parameters, and computes the actual SPI clock 
 * frequency in Hz.
 * 
 * @param[in] spi A pointer to the SPI structure.
 * 
 * @return The actual SPI clock frequency in Hz.
 * 
 * @note The SPI clock frequency is calculated based on the clock source, 
 *       clock divider (n and m), and the configured clock source.
 * 
 * @note The function prints a trace message with the computed SPI clock frequency,
 *       the register value, and the clock divider values (n and m).
 */
static int sunxi_spi_get_clk(sunxi_spi_t *spi) {
	uint32_t reg_val = 0;
	uint32_t src = 0, clk = 0, sclk_freq = 0;
	uint32_t n, m;

	// Read the SPI clock configuration register.
	reg_val = read32(spi->spi_clk.spi_clock_cfg_base);

	// Extract the clock source (src), clock divider (n), and multiplier (m) from the register.
	src = (reg_val >> 24) & 0x7;
	n = (reg_val >> spi->spi_clk.spi_clock_factor_n_offset) & 0x3;
	m = ((reg_val >> 0) & 0xf) + 1;

	// Determine the clock source based on the extracted value (src).
	switch (src) {
		case 0:
			clk = 24000000;// Source clock is 24 MHz (likely an external reference clock).
			break;
		case 1:
		case 2:
			clk = spi->parent_clk_reg.parent_clk;// Use parent clock.
			break;
		default:
			clk = 0;// Invalid clock source.
			break;
	}

	// Calculate the actual SPI clock frequency using the clock source, divider (n), and multiplier (m).
	sclk_freq = clk / (1 << n) / m;

	// Print trace message with SPI clock frequency and register values for debugging.
	printk_debug("SPI: sclk_freq= %d Hz, reg_val: 0x%08x , n=%d, m=%d\n", sclk_freq, reg_val, n, m);

	return sclk_freq;// Return the calculated SPI clock frequency in Hz.
}

/**
 * @brief Initializes the SPI clock.
 * 
 * This function configures the SPI clock by setting the appropriate divisor and clock source,
 * enabling the SPI clock, performing a reset on the SPI, and setting up the clock gating.
 * 
 * @param spi Pointer to the SPI structure containing configuration and register information.
 */
void __attribute__((weak)) sunxi_spi_clk_init(sunxi_spi_t *spi) {
	uint32_t div, source_clk, mod_clk, n, m;
	uint32_t reg_val;

	/* close gate */
	clrbits_le32(spi->parent_clk_reg.gate_reg_base, BIT(spi->parent_clk_reg.gate_reg_offset));

	source_clk = spi->parent_clk_reg.parent_clk;
	mod_clk = spi->clk_rate;

	div = ((source_clk + mod_clk) / mod_clk) - 1;
	div = div == 0 ? 1 : div;
	if (div > 128) {
		m = 1;
		n = 0;
		return;
	} else if (div > 64) {
		n = 3;
		m = div >> 3;
	} else if (div > 32) {
		n = 2;
		m = div >> 2;
	} else if (div > 16) {
		n = 1;
		m = div >> 1;
	} else {
		n = 0;
		m = div;
	}

	printk_debug("SPI: SPI%d clk parent %uHz, mclk=0x%08x, n=%u, m=%u\n", spi->id, source_clk, readl(spi->spi_clk.spi_clock_cfg_base), n, m);

	/* set m factor, factor_m = m -1 */
	m -= 1;

	reg_val = BIT(31) | (spi->spi_clk.spi_clock_source << 24) | (n << spi->spi_clk.spi_clock_factor_n_offset) | m;

	/* enable spi clock */
	write32(spi->spi_clk.spi_clock_cfg_base, reg_val);

	/* SPI Reset */
	clrbits_le32(spi->parent_clk_reg.rst_reg_base, BIT(spi->parent_clk_reg.rst_reg_offset));
	udelay(1);
	setbits_le32(spi->parent_clk_reg.rst_reg_base, BIT(spi->parent_clk_reg.rst_reg_offset));

	/* SPI gating */
	setbits_le32(spi->parent_clk_reg.gate_reg_base, BIT(spi->parent_clk_reg.gate_reg_offset));

	spi->spi_clk.spi_clock_freq = sunxi_spi_get_clk(spi);
}

/**
 * @brief Deinitializes the SPI clock.
 * 
 * This function disables the SPI clock, clears the clock gating, and asserts the SPI reset.
 * 
 * @param spi Pointer to the SPI structure containing configuration and register information.
 */
static void sunxi_spi_clk_deinit(sunxi_spi_t *spi) {
	/* SPI Assert */
	clrbits_le32(spi->parent_clk_reg.rst_reg_base, BIT(spi->parent_clk_reg.rst_reg_offset));

	/* SPI Close gating */
	clrbits_le32(spi->parent_clk_reg.gate_reg_base, BIT(spi->parent_clk_reg.gate_reg_offset));
}

/**
 * @brief Configures the SPI transfer control settings.
 * 
 * This function configures the transfer control register (`tc`) based on the SPI clock frequency.
 * The transfer control settings such as data width, polarity, and phase are set accordingly.
 * 
 * @param spi Pointer to the SPI structure containing configuration and register information.
 */
static void sunxi_spi_config_transer_control(sunxi_spi_t *spi) {
	sunxi_spi_reg_t *spi_reg = (sunxi_spi_reg_t *) spi->base;

	uint32_t reg_val = spi_reg->tc;

	if (spi->spi_clk.spi_clock_freq > SPI_HIGH_FREQUENCY) {
		reg_val &= ~(SPI_TC_SDC | SPI_TC_SDM);
		reg_val |= SPI_TC_SDC;
	} else if (spi->spi_clk.spi_clock_freq <= SPI_LOW_FREQUENCY) {
		reg_val &= ~(SPI_TC_SDC | SPI_TC_SDM);
		reg_val |= SPI_TC_SDM;
	} else {
		reg_val &= ~(SPI_TC_SDC | SPI_TC_SDM);
	}
	reg_val |= SPI_TC_DHB | SPI_TC_SS_LEVEL | SPI_TC_SPOL;

	spi_reg->tc = reg_val;
}

/**
 * @brief Sets the SPI I/O mode.
 * 
 * This function configures the I/O mode for SPI data transfer, including single, dual, or quad mode.
 * 
 * @param spi Pointer to the SPI structure containing configuration and register information.
 * @param mode The desired SPI I/O mode (e.g., SPI_IO_SINGLE, SPI_IO_DUAL_RX, SPI_IO_QUAD_RX, etc.).
 */
static void sunxi_spi_set_io_mode(sunxi_spi_t *spi, spi_io_mode_t mode) {
	sunxi_spi_reg_t *spi_reg = (sunxi_spi_reg_t *) spi->base;

	spi_reg->bcc &= ~(SPI_BCC_QUAD_MODE | SPI_BCC_DUAL_MODE);
	switch (mode) {
		case SPI_IO_DUAL_RX:
			spi_reg->bcc |= SPI_BCC_DUAL_MODE;
			break;
		case SPI_IO_QUAD_RX:
		case SPI_IO_QUAD_IO:
			spi_reg->bcc |= SPI_BCC_QUAD_MODE;
			break;
		case SPI_IO_SINGLE:
		default:
			break;
	}
}

/**
 * @brief Sets the counters for SPI transmission.
 * 
 * This function sets the burst, transmit, and dummy counters for SPI data transmission. The counters 
 * are configured based on the transmission length, reception length, start transmission length, and dummy data length.
 * 
 * @param spi Pointer to the SPI structure containing configuration and register information.
 * @param txlen The length of the transmission data in bytes.
 * @param rxlen The length of the reception data in bytes.
 * @param stxlen The length of the start transmission data in bytes.
 * @param dummylen The length of the dummy data in bytes.
 */
static void sunxi_spi_set_counters(sunxi_spi_t *spi, int txlen, int rxlen, int stxlen, int dummylen) {
	sunxi_spi_reg_t *spi_reg = (sunxi_spi_reg_t *) spi->base;
	uint32_t val;

	val = spi_reg->burst_cnt;
	val &= ~SPI_BC_CNT_MASK;
	val |= (SPI_BC_CNT_MASK & (txlen + rxlen + dummylen));
	spi_reg->burst_cnt = val;

	val = spi_reg->transmit_cnt;
	val &= ~SPI_TC_CNT_MASK;
	val |= (SPI_TC_CNT_MASK & txlen);
	spi_reg->transmit_cnt = val;

	val = spi_reg->bcc;
	val &= ~SPI_BCC_STC_MASK;
	val |= (SPI_BCC_STC_MASK & stxlen);
	val &= ~SPI_BCC_DBC_MASK;
	val |= (dummylen << SPI_BCC_DBC_POS);
	spi_reg->bcc = val;
}

/**
 * @brief Initializes the SPI bus.
 * 
 * This function performs the necessary initialization steps for the SPI bus, including performing a 
 * soft reset, enabling the bus, configuring the chip select (CS), setting the SPI mode to master, 
 * configuring the clock, transfer control, and setting other SPI bus settings like slave select level 
 * and transmit pause.
 * 
 * @param spi Pointer to the SPI structure containing configuration and register information.
 */
static void sunxi_spi_bus_init(sunxi_spi_t *spi) {
	sunxi_spi_soft_reset(spi);

	sunxi_spi_enable_bus(spi);

	sunxi_spi_set_cs(spi, 0);

	sunxi_spi_set_master(spi);

	sunxi_spi_set_clk(spi, spi->clk_rate, spi->spi_clk.spi_clock_freq, spi->spi_clk.cdr_mode);

	sunxi_spi_config_transer_control(spi);

	sunxi_spi_set_ss_level(spi, 1);

	sunxi_spi_enable_transmit_pause(spi);

	sunxi_spi_set_ss_owner(spi, 0);

	sunxi_spi_reset_fifo(spi);
}

/**
 * @brief Configures the GPIO pins for SPI communication.
 * 
 * This function configures the SPI-related GPIO pins, including chip select (CS), clock (SCK), 
 * master-out slave-in (MOSI), master-in slave-out (MISO), write protect (WP), and hold pins.
 * It sets the appropriate multiplexing functions and default pull-up resistors for the WP and hold pins.
 * 
 * @param spi Pointer to the SPI structure containing the GPIO configuration.
 */
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
int sunxi_spi_init(sunxi_spi_t *spi) {
	/* if set dma handle, we using dma mode */
	if (spi->dma_handle != NULL) {
		sunxi_spi_dma_init(spi);
	}

	sunxi_spi_gpio_init(spi);

	sunxi_spi_clk_init(spi);

	sunxi_spi_bus_init(spi);

	return 0;
}

/**
 * @brief Disables the SPI interface.
 * 
 * This function disables the SPI bus, deinitializes DMA (if used), and deinitializes the SPI clock. 
 * It is called when the SPI interface is no longer needed or when performing cleanup operations.
 * 
 * @param spi Pointer to the SPI structure containing configuration and register information.
 */
void sunxi_spi_disable(sunxi_spi_t *spi) {
	sunxi_spi_disable_bus(spi); /**< Disable the SPI bus */
	sunxi_spi_dma_deinit(spi);	/**< Deinitialize the DMA */
	sunxi_spi_clk_deinit(spi);	/**< Deinitialize the SPI clock */
}

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
int sunxi_spi_update_clk(sunxi_spi_t *spi, uint32_t clk) {
	spi->clk_rate = clk;				   /**< Update the SPI clock rate */
	sunxi_spi_clk_init(spi);			   /**< Reinitialize the SPI clock with the new rate */
	sunxi_spi_bus_init(spi);			   /**< Reinitialize the SPI bus */
	sunxi_spi_config_transer_control(spi); /**< Reconfigure the transfer control */
	return 0;							   /**< Return success */
}

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
int sunxi_spi_transfer(sunxi_spi_t *spi, spi_io_mode_t mode, void *txbuf, uint32_t txlen, void *rxbuf, uint32_t rxlen) {
	sunxi_spi_reg_t *spi_reg = (sunxi_spi_reg_t *) spi->base;
	uint32_t stxlen;

	printk_trace("SPI: tsfr mode=%u tx=%u rx=%u\n", mode, txlen, rxlen);

	sunxi_spi_disable_irq(spi, SPI_INT_STA_PENDING_BIT);	 /**< Disable interrupt for pending status */
	sunxi_spi_clr_irq_pending(spi, SPI_INT_STA_PENDING_BIT); /**< Clear any pending interrupt */

	sunxi_spi_set_io_mode(spi, mode); /**< Set the I/O mode (single, dual, quad) */

	switch (mode) {
		case SPI_IO_QUAD_IO:
			stxlen = 1; /**< Only opcode in quad mode */
			break;
		case SPI_IO_DUAL_RX:
		case SPI_IO_QUAD_RX:
			stxlen = txlen; /**< Only transmit data in dual or quad RX mode */
			break;
		case SPI_IO_SINGLE:
		default:
			stxlen = txlen + rxlen; /**< Both transmit and receive data in single mode */
			break;
	}

	sunxi_spi_set_counters(spi, txlen, rxlen, stxlen, 0); /**< Set the SPI transfer counters */
	sunxi_spi_reset_fifo(spi);							  /**< Reset the SPI FIFOs */
	sunxi_spi_start_xfer(spi);							  /**< Start the SPI transfer */

	if (txbuf && txlen) {
		sunxi_spi_write_tx_fifo(spi, txbuf, txlen); /**< Write data to TX FIFO if there's data to transmit */
	}

	if (rxbuf && rxlen) {
		if (rxlen > 64) {
			sunxi_spi_read_by_dma(spi, rxbuf, rxlen); /**< Use DMA for large receive buffers */
		} else {
			sunxi_spi_read_rx_fifo(spi, rxbuf, rxlen); /**< Use FIFO for smaller receive buffers */
		}
	}

	if (sunxi_spi_query_irq_pending(spi) & SPI_INT_STA_ERR) {
		printk_warning("SPI: int sta err\n"); /**< Check for error interrupt */
	}

	while (!(sunxi_spi_query_irq_pending(spi) & SPI_INT_STA_TC))
		; /**< Wait for transfer completion interrupt (TC) */

	sunxi_spi_dma_disable(spi); /**< Disable DMA if used */

	if (spi_reg->burst_cnt == 0) {
		if (spi_reg->tc & SPI_TC_XCH) {
			printk_warning("SPI: XCH Control failed\n"); /**< Warn if exchange control fails */
		}
	} else {
		printk_warning("SPI: MBC error\n"); /**< Warn if there is an MBC error (memory-to-bus control) */
	}

	sunxi_spi_clr_irq_pending(spi, SPI_INT_STA_PENDING_BIT); /**< Clear any pending interrupt */

	printk_trace("SPI: ISR=0x%x\n", spi_reg->int_sta); /**< Log the current interrupt status register */

	return rxlen + txlen; /**< Return the total number of transferred bytes (TX + RX) */
}
