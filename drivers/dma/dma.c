/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file
 * @brief System DMA (Direct Memory Access) driver for Allwinner (sunxi) platforms
 * @details This file implements DMA functionality for transferring data between
 *          memory and peripherals without CPU intervention, supporting multiple
 *          channels, interrupt handling, and various DMA operations.
 */

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>

#include <driver.h>
#include <drivers/dma.h>
#include <dt2c/driver.h>

/**
 * @brief Initialize DMA clock
 * @details Configures the clock settings for the DMA controller, including bus clock gating,
 *          DMA reset, and DMA clock gating.
 * @param dma Pointer to DMA configuration structure containing clock register information
 */
void sunxi_dma_clk_init(sunxi_dma_t *dma) {
	/* DMA : mbus clock gating */
	setbits_le32(dma->bus_clk.gate_reg_base, BIT(dma->bus_clk.gate_reg_offset));
	/* DMA reset */
	setbits_le32(dma->dma_clk.rst_reg_base, BIT(dma->dma_clk.rst_reg_offset));
	/* DMA gating */
	setbits_le32(dma->dma_clk.gate_reg_base, BIT(dma->dma_clk.gate_reg_offset));
}

/**
 * @brief Initialize the DMA controller
 * @details Initializes the DMA controller by setting up registers, clearing interrupts,
 *          configuring auto clock gating, and initializing channel structures.
 * @param dma Pointer to DMA configuration structure containing base addresses and settings
 */
void sunxi_dma_init(sunxi_dma_t *dma) {
	int i = 0;
	sunxi_dma_reg_t *dma_reg;

	if (dma == NULL || dma->dma_reg_base == 0U || dma->initialized)
		return;
	dma_reg = (sunxi_dma_reg_t *) dma->dma_reg_base;

	sunxi_dma_clk_init(dma);

	// Disable all interrupts
	dma_reg->irq_en0 = 0;
	dma_reg->irq_en1 = 0;

	// Clear all pending interrupts
	dma_reg->irq_pending0 = 0xffffffff;
	dma_reg->irq_pending1 = 0xffffffff;

	/* Disable auto MCLK gating */
	dma_reg->auto_gate &= ~(0x7 << 0);
	dma_reg->auto_gate |= 0x7 << 0;

	// Initialize all channel sources to zero
	memset((void *) dma->channel_source, 0,
	       SUNXI_DMA_MAX * sizeof(sunxi_dma_source_t));

	// Set up each DMA channel
	for (i = 0; i < SUNXI_DMA_MAX; i++) {
		dma->channel_source[i].used = 0;
		dma->channel_source[i].channel = &(dma_reg->channel[i]);
		dma->channel_source[i].desc = &dma->channel_desc[i];
		dma->channel_source[i].owner = dma;
	}

	dma->interrupt_count = 0;
	dma->initialized = true;
}

/**
 * @brief Exit and clean up DMA resources
 * @details Releases all DMA channels, disables interrupts, and shuts down the DMA controller.
 * @param dma Pointer to DMA configuration structure
 */
void sunxi_dma_exit(sunxi_dma_t *dma) {
	uintptr_t dma_fd;
	sunxi_dma_reg_t *dma_reg;

	if (dma == NULL || !dma->initialized)
		return;
	dma_reg = (sunxi_dma_reg_t *) dma->dma_reg_base;

	/* Free any DMA channels that haven't been released */
	for (int i = 0; i < SUNXI_DMA_MAX; i++) {
		if (dma->channel_source[i].used == 1) {
			dma->channel_source[i].channel->enable = 0;
			dma_fd = (uintptr_t) &dma->channel_source[i];
			sunxi_dma_disable_int(dma_fd);
			sunxi_dma_free_int(dma_fd);
			dma->channel_source[i].used = 0;
		}
	}

	/* Close DMA clock when exiting */
	dma_reg->auto_gate &= ~(1 << dma->dma_clk.gate_reg_offset | 1 << dma->dma_clk.rst_reg_offset);

	// Disable all interrupts
	dma_reg->irq_en0 = 0;
	dma_reg->irq_en1 = 0;

	// Clear all pending interrupts
	dma_reg->irq_pending0 = 0xffffffff;
	dma_reg->irq_pending1 = 0xffffffff;

	dma->initialized = false;
}

/**
 * @brief Request a DMA channel starting from the last available
 * @details Searches for an available DMA channel starting from the highest-numbered channel.
 * @param dmatype Type of DMA channel to request (currently unused)
 * @return Handle to the requested DMA channel, or 0 if no channels are available
 */
uintptr_t sunxi_dma_request_from_last(sunxi_dma_t *dma, uint32_t dmatype) {
	(void) dmatype;
	if (dma == NULL || !dma->initialized)
		return 0;
	for (int i = SUNXI_DMA_MAX - 1; i >= 0; i--) {
		if (dma->channel_source[i].used == 0) {
			dma->channel_source[i].used = 1;
			dma->channel_source[i].channel_count = i;
			return (uintptr_t) &dma->channel_source[i];
		}
	}

	return 0;
}

/**
 * @brief Request a DMA channel
 * @details Searches for an available DMA channel starting from channel 0.
 * @param dmatype Type of DMA channel to request (currently unused)
 * @return Handle to the requested DMA channel, or 0 if no channels are available
 */
uintptr_t sunxi_dma_request(sunxi_dma_t *dma, uint32_t dmatype) {
	(void) dmatype;
	if (dma == NULL || !dma->initialized)
		return 0;
	for (int i = 0; i < SUNXI_DMA_MAX; i++) {
		if (dma->channel_source[i].used == 0) {
			dma->channel_source[i].used = 1;
			dma->channel_source[i].channel_count = i;
			printk_debug("DMA: provide channel %u\n", i);
			return (uintptr_t) &dma->channel_source[i];
		}
	}

	return 0;
}

/**
 * @brief Release a DMA channel
 * @details Releases a previously requested DMA channel, disabling its interrupts.
 * @param dma_fd Handle to the DMA channel to release
 * @return 0 on success, -1 if the channel was not in use
 */
int sunxi_dma_release(uintptr_t dma_fd) {
	sunxi_dma_source_t *dma_source = (sunxi_dma_source_t *) dma_fd;

	if (dma_source == NULL || !dma_source->used || dma_source->owner == NULL) {
		return -1;
	}

	// Disable and free interrupts
	sunxi_dma_disable_int(dma_fd);
	sunxi_dma_free_int(dma_fd);

	// Mark channel as unused
	dma_source->used = 0;

	return 0;
}

/**
 * @brief Configure DMA channel settings
 * @details Sets up a DMA channel with specified configuration parameters like loop mode,
 *          wait cycles, and data block size.
 * @param dma_fd Handle to the DMA channel to configure
 * @param cfg Pointer to configuration structure containing DMA settings
 * @return 0 on success, -1 if the channel is not in use
 */
int sunxi_dma_setting(uintptr_t dma_fd, sunxi_dma_set_t *cfg) {
	uint32_t commit_para;
	sunxi_dma_set_t *dma_set = cfg;
	sunxi_dma_source_t *dma_source = (sunxi_dma_source_t *) dma_fd;
	sunxi_dma_desc_t *desc;

	if (dma_source == NULL || dma_set == NULL || !dma_source->used ||
	    dma_source->owner == NULL)
		return -1;
	desc = dma_source->desc;

	// Configure descriptor link for loop mode
	if (dma_set->loop_mode)
		desc->link = (uint32_t) (uintptr_t) dma_source->desc;
	else
		desc->link = SUNXI_DMA_LINK_NULL;

	// Set commit parameters
	commit_para = (dma_set->wait_cyc & 0xff);
	commit_para |= (dma_set->data_block_size & 0xff) << 8;

	desc->commit_para = commit_para;
	memcpy((void *) &desc->config, &dma_set->channel_cfg, sizeof(desc->config));

	return 0;
}

/**
 * @brief Start a DMA transfer
 * @details Initiates a DMA data transfer from source to destination address with specified size.
 * @param dma_fd Handle to the DMA channel to use
 * @param saddr Source address for the DMA transfer
 * @param daddr Destination address for the DMA transfer
 * @param bytes Number of bytes to transfer
 * @return 0 on success, -1 if the channel is not in use
 */
int sunxi_dma_start(uintptr_t dma_fd, uintptr_t saddr, uintptr_t daddr, uint32_t bytes) {
	sunxi_dma_source_t *dma_source = (sunxi_dma_source_t *) dma_fd;
	sunxi_dma_channel_reg_t *channel;
	sunxi_dma_desc_t *desc;

	if (dma_source == NULL || !dma_source->used || dma_source->owner == NULL)
		return -1;
	channel = dma_source->channel;
	desc = dma_source->desc;

	/* Configure descriptor */
	desc->source_addr = (uint32_t) saddr;
	desc->dest_addr = (uint32_t) daddr;
	desc->byte_count = bytes;

	/* Start DMA transfer */
	channel->desc_addr = (uint32_t) (uintptr_t) desc;
	channel->enable = 1;

	return 0;
}

/**
 * @brief Stop a DMA transfer
 * @details Halts an ongoing DMA transfer on the specified channel.
 * @param dma_fd Handle to the DMA channel to stop
 * @return 0 on success, -1 if the channel is not in use
 */
int sunxi_dma_stop(uintptr_t dma_fd) {
	sunxi_dma_source_t *dma_source = (sunxi_dma_source_t *) dma_fd;
	sunxi_dma_channel_reg_t *channel;

	if (dma_source == NULL || !dma_source->used || dma_source->owner == NULL)
		return -1;
	channel = dma_source->channel;
	
	// Disable the channel
	channel->enable = 0;

	return 0;
}

/**
 * @brief Query DMA transfer status
 * @details Checks whether a DMA transfer is still in progress on the specified channel.
 * @param dma_fd Handle to the DMA channel to query
 * @return 1 if transfer is in progress, 0 if completed, -1 if channel is not in use
 */
int sunxi_dma_querystatus(uintptr_t dma_fd) {
	uint32_t channel_count;
	sunxi_dma_source_t *dma_source = (sunxi_dma_source_t *) dma_fd;
	sunxi_dma_reg_t *dma_reg;

	if (dma_source == NULL || !dma_source->used || dma_source->owner == NULL)
		return -1;
	dma_reg = (sunxi_dma_reg_t *) dma_source->owner->dma_reg_base;

	channel_count = dma_source->channel_count;

	// Return the status bit for this channel
	return (dma_reg->status >> channel_count) & 0x01;
}

/**
 * @brief Install interrupt handler for DMA channel
 * @details Sets up an interrupt handler for a DMA channel and clears any pending interrupts.
 * @param dma_fd Handle to the DMA channel
 * @param p User data to associate with the interrupt handler
 * @return 0 on success, -1 if channel is not in use
 */
int sunxi_dma_install_int(uintptr_t dma_fd, void *p) {
	sunxi_dma_source_t *dma_source = (sunxi_dma_source_t *) dma_fd;
	sunxi_dma_reg_t *dma_reg;
	uint8_t channel_count;

	if (dma_source == NULL || !dma_source->used || dma_source->owner == NULL)
		return -1;
	dma_reg = (sunxi_dma_reg_t *) dma_source->owner->dma_reg_base;

	channel_count = dma_source->channel_count;

	// Clear pending interrupts for this channel
	if (channel_count < 8)
		dma_reg->irq_pending0 = (7 << channel_count * 4);
	else
		dma_reg->irq_pending1 = (7 << (channel_count - 8) * 4);

	// Set up interrupt function if not already set
	if (!dma_source->dma_func.m_func) {
		dma_source->dma_func.m_func = 0;
		dma_source->dma_func.m_data = p;
	} else {
		printk_error("DMA: %p int is used already, you have to free it first\n", (void *) dma_fd);
	}

	return 0;
}

/**
 * @brief Enable interrupts for a DMA channel
 * @details Enables interrupts for a specific DMA channel, specifically package end interrupts.
 * @param dma_fd Handle to the DMA channel
 * @return 0 on success, -1 if channel is not in use
 */
int sunxi_dma_enable_int(uintptr_t dma_fd) {
	sunxi_dma_source_t *dma_source = (sunxi_dma_source_t *) dma_fd;
	sunxi_dma_reg_t *dma_reg;
	uint8_t channel_count;

	if (dma_source == NULL || !dma_source->used || dma_source->owner == NULL)
		return -1;
	dma_reg = (sunxi_dma_reg_t *) dma_source->owner->dma_reg_base;

	channel_count = dma_source->channel_count;
	if (channel_count < 8) {
		// Check if interrupt is already enabled
		if ((dma_reg->irq_en0) & (DMA_PKG_END_INT << channel_count * 4)) {
			printk_debug("DMA: %p int is available already\n", (void *) dma_fd);
			return 0;
		}
		// Enable package end interrupt for this channel
		dma_reg->irq_en0 |= (DMA_PKG_END_INT << channel_count * 4);
	} else {
		// Check if interrupt is already enabled
		if ((dma_reg->irq_en1) & (DMA_PKG_END_INT << (channel_count - 8) * 4)) {
			printk_debug("DMA: %p int is available already\n", (void *) dma_fd);
			return 0;
		}
		// Enable package end interrupt for this channel
		dma_reg->irq_en1 |= (DMA_PKG_END_INT << (channel_count - 8) * 4);
	}

	// Increment interrupt count
	dma_source->owner->interrupt_count++;

	return 0;
}

/**
 * @brief Disable interrupts for a DMA channel
 * @details Disables interrupts for a specific DMA channel.
 * @param dma_fd Handle to the DMA channel
 * @return 0 on success, -1 if channel is not in use
 */
int sunxi_dma_disable_int(uintptr_t dma_fd) {
	sunxi_dma_source_t *dma_source = (sunxi_dma_source_t *) dma_fd;
	sunxi_dma_reg_t *dma_reg;
	uint8_t channel_count;

	if (dma_source == NULL || !dma_source->used || dma_source->owner == NULL)
		return -1;
	dma_reg = (sunxi_dma_reg_t *) dma_source->owner->dma_reg_base;

	channel_count = dma_source->channel_count;
	if (channel_count < 8) {
		// Check if interrupt is not enabled
		if (!((dma_reg->irq_en0) & (DMA_PKG_END_INT << channel_count * 4))) {
			printk_debug("DMA: %p int is not used yet\n", (void *) dma_fd);
			return 0;
		}
		// Disable package end interrupt for this channel
		dma_reg->irq_en0 &= ~(DMA_PKG_END_INT << channel_count * 4);
	} else {
		// Check if interrupt is not enabled
		if (!((dma_reg->irq_en1) & (DMA_PKG_END_INT << (channel_count - 8) * 4))) {
			printk_debug("DMA: %p int is not used yet\n", (void *) dma_fd);
			return 0;
		}
		// Disable package end interrupt for this channel
		dma_reg->irq_en1 &= ~(DMA_PKG_END_INT << (channel_count - 8) * 4);
	}

	/* Decrement interrupt count */
	if (dma_source->owner->interrupt_count > 0)
		dma_source->owner->interrupt_count--;

	return 0;
}

/**
 * @brief Free interrupt handler for a DMA channel
 * @details Releases the interrupt handler associated with a DMA channel and clears pending interrupts.
 * @param dma_fd Handle to the DMA channel
 * @return 0 on success, -1 if channel is not in use or interrupt handler was not set
 */
int sunxi_dma_free_int(uintptr_t dma_fd) {
	sunxi_dma_source_t *dma_source = (sunxi_dma_source_t *) dma_fd;
	sunxi_dma_reg_t *dma_reg;
	uint8_t channel_count;

	if (dma_source == NULL || !dma_source->used || dma_source->owner == NULL)
		return -1;
	dma_reg = (sunxi_dma_reg_t *) dma_source->owner->dma_reg_base;

	channel_count = dma_source->channel_count;
	// Clear pending interrupts
	if (channel_count < 8)
		dma_reg->irq_pending0 = (7 << channel_count);
	else
		dma_reg->irq_pending1 = (7 << (channel_count - 8));

	// Free the interrupt handler if set
	if (dma_source->dma_func.m_func) {
		dma_source->dma_func.m_func = NULL;
		dma_source->dma_func.m_data = NULL;
	} else {
		printk_debug("DMA: %p int is free, you do not need to free it again\n", (void *) dma_fd);
		return -1;
	}

	return 0;
}

/**
 * @brief Test DMA functionality
 * @details Performs a DMA transfer test between two memory regions, verifies data integrity,
 *          and reports transfer performance.
 * @param dma DMA controller used for the test transfer
 * @param src_addr Source memory address for test transfer
 * @param dst_addr Destination memory address for test transfer
 * @param len Length of data to transfer in bytes
 * @return 0 on success, -1 if channel request failed, -2 if transfer timed out
 */
int sunxi_dma_test(sunxi_dma_t *dma, uint32_t *src_addr, uint32_t *dst_addr,
		   uint32_t len) {
	sunxi_dma_set_t dma_set;
	uint32_t st = 0;
	uint32_t timeout;
	uintptr_t dma_fd;
	uint32_t i, valid;

	if (dma == NULL)
		return -1;
	sunxi_dma_init(dma);
	if (!dma->initialized)
		return -1;

	// Align length to 4-byte boundary
	len = ALIGN(len, 4);
	printk_debug("DMA: test %p ====> %p, len %uKB \n", src_addr, dst_addr, (len / 1024));

	/* Configure DMA settings */
	dma_set.loop_mode = 0;
	dma_set.wait_cyc = 8;
	dma_set.data_block_size = 1 * 32 / 8;
	
	/* Channel configuration (DRAM to DRAM transfer) */
	dma_set.channel_cfg.src_drq_type = DMAC_CFG_TYPE_DRAM; // DRAM source
	dma_set.channel_cfg.src_addr_mode = DMAC_CFG_DEST_ADDR_TYPE_LINEAR_MODE;
	dma_set.channel_cfg.src_burst_length = DMAC_CFG_SRC_4_BURST;
	dma_set.channel_cfg.src_data_width = DMAC_CFG_SRC_DATA_WIDTH_16BIT;
	dma_set.channel_cfg.reserved0 = 0;

	dma_set.channel_cfg.dst_drq_type = DMAC_CFG_TYPE_DRAM; // DRAM destination
	dma_set.channel_cfg.dst_addr_mode = DMAC_CFG_DEST_ADDR_TYPE_LINEAR_MODE;
	dma_set.channel_cfg.dst_burst_length = DMAC_CFG_DEST_4_BURST;
	dma_set.channel_cfg.dst_data_width = DMAC_CFG_DEST_DATA_WIDTH_16BIT;
	dma_set.channel_cfg.reserved1 = 0;

	// Request a DMA channel
	dma_fd = sunxi_dma_request(dma, 0);
	if (!dma_fd) {
		printk_error("DMA: can't request dma\n");
		return -1;
	}

	// Configure the DMA channel
	sunxi_dma_setting(dma_fd, &dma_set);

	// Prepare test data
	for (i = 0; i < (len / 4); i += 4) {
		src_addr[i] = i;
		src_addr[i + 1] = i + 1;
		src_addr[i + 2] = i + 2;
		src_addr[i + 3] = i + 3;
	}

	/* Set timeout to 100 ms */
	timeout = time_ms();

	// Start the DMA transfer
	sunxi_dma_start(dma_fd, (uintptr_t) src_addr, (uintptr_t) dst_addr, len);
	st = sunxi_dma_querystatus(dma_fd);

	// Wait for transfer to complete or timeout
	while ((time_ms() - timeout < 100) && st) { 
		st = sunxi_dma_querystatus(dma_fd); 
	}

	if (st) {
		// Transfer timed out
		printk_error("DMA: test timeout!\n");
		sunxi_dma_stop(dma_fd);
		sunxi_dma_release(dma_fd);
		return -2;
	} else {
		// Verify data integrity
		valid = 1;
		printk_debug("DMA: test done in %lums\n", (time_ms() - timeout));
		
		for (i = 0; i < (len / 4); i += 4) {
			if (dst_addr[i] != i || dst_addr[i + 1] != i + 1 || 
			    dst_addr[i + 2] != i + 2 || dst_addr[i + 3] != i + 3) {
				valid = 0;
				break;
			}
		}
		
		if (valid)
			printk_debug("DMA: test check valid\n");
		else
			printk_error("DMA: test check failed at %u bytes\n", i);
	}

	// Clean up
	sunxi_dma_stop(dma_fd);
	sunxi_dma_release(dma_fd);

	return 0;
}

DT2C_DRIVER_COMPAT("allwinner,sunxi-dma");
