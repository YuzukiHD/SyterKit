/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef _SUNXI_DMA_H
#define _SUNXI_DMA_H

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <common.h>
#include <log.h>

#include <reg-dma.h>
#include <sys-clk.h>

#ifdef __cplusplus
extern "C" {
#endif// __cplusplus

typedef struct {
	uint32_t volatile config;
	uint32_t volatile source_addr;
	uint32_t volatile dest_addr;
	uint32_t volatile byte_count;
	uint32_t volatile commit_para;
	uint32_t volatile link;
	uint32_t volatile reserved[2];
} sunxi_dma_desc_t;

typedef struct {
	uint32_t volatile src_drq_type : 6;
	uint32_t volatile src_burst_length : 2;
	uint32_t volatile src_addr_mode : 1;
	uint32_t volatile src_data_width : 2;
	uint32_t volatile reserved0 : 5;
	uint32_t volatile dst_drq_type : 6;
	uint32_t volatile dst_burst_length : 2;
	uint32_t volatile dst_addr_mode : 1;
	uint32_t volatile dst_data_width : 2;
	uint32_t volatile reserved1 : 5;
} sunxi_dma_channel_config_t;

typedef struct {
	sunxi_dma_channel_config_t channel_cfg;
	uint32_t loop_mode;
	uint32_t data_block_size;
	uint32_t wait_cyc;
} sunxi_dma_set_t;

typedef struct {
	void *m_data;
	void (*m_func)(void);
} sunxi_dma_irq_handler_t;

typedef struct {
	uint32_t volatile enable;
	uint32_t volatile pause;
	uint32_t volatile desc_addr;
	uint32_t volatile config;
	uint32_t volatile cur_src_addr;
	uint32_t volatile cur_dst_addr;
	uint32_t volatile left_bytes;
	uint32_t volatile parameters;
	uint32_t volatile mode;
	uint32_t volatile fdesc_addr;
	uint32_t volatile pkg_num;
	uint32_t volatile res[5];
} sunxi_dma_channel_reg_t;

typedef struct {
	uint32_t volatile irq_en0; /* 0x0 dma irq enable register 0 */
	uint32_t volatile irq_en1; /* 0x4 dma irq enable register 1 */
	uint32_t volatile reserved0[2];
	uint32_t volatile irq_pending0; /* 0x10 dma irq pending register 0 */
	uint32_t volatile irq_pending1; /* 0x14 dma irq pending register 1 */
	uint32_t volatile reserved1[2];
	uint32_t volatile security; /* 0x20 dma security register */
	uint32_t volatile reserved3[1];
	uint32_t volatile auto_gate; /* 0x28 dma auto gating register */
	uint32_t volatile reserved4[1];
	uint32_t volatile status; /* 0x30 dma status register */
	uint32_t volatile reserved5[3];
	uint32_t volatile version; /* 0x40 dma Version register */
	uint32_t volatile reserved6[47];
	sunxi_dma_channel_reg_t channel[16]; /* 0x100 dma channel register */
} sunxi_dma_reg_t;

typedef struct {
	uint32_t used;
	uint32_t channel_count;
	sunxi_dma_channel_reg_t *channel;
	uint32_t reserved;
	sunxi_dma_desc_t *desc;
	sunxi_dma_irq_handler_t dma_func;
} sunxi_dma_source_t;

typedef struct {
	uint32_t dma_reg_base;
	sunxi_clk_t dma_clk;
	sunxi_clk_t bus_clk;
} sunxi_dma_t;

#define DMA_DEFAULT_CLK_RST_OFFSET (16)
#define DMA_DEFAULT_CLK_GATE_OFFSET (0)

/**
 * Initialize the DMA subsystem.
 */
void sunxi_dma_init(sunxi_dma_t *dma);

/**
 * Clean up and exit the DMA subsystem.
 */
void sunxi_dma_exit(sunxi_dma_t *dma);

/**
 * Request a DMA channel of the specified type.
 *
 * @param dmatype The type of DMA channel to request.
 * @return The DMA channel number if successful, or an error code if failed.
 */
uint32_t sunxi_dma_request(uint32_t dmatype);

/**
 * Request a DMA channel from the last allocated channel of the specified type.
 *
 * @param dmatype The type of DMA channel to request.
 * @return The DMA channel number if successful, or an error code if failed.
 */
uint32_t sunxi_dma_request_from_last(uint32_t dmatype);

/**
 * Release a previously requested DMA channel.
 *
 * @param hdma The DMA channel number to release.
 * @return 0 if successful, or an error code if failed.
 */
int sunxi_dma_release(uint32_t dma_fd);

/**
 * Configure the settings of a DMA channel.
 *
 * @param hdma The DMA channel number to configure.
 * @param cfg Pointer to the DMA configuration structure.
 * @return 0 if successful, or an error code if failed.
 */
int sunxi_dma_setting(uint32_t dma_fd, sunxi_dma_set_t *cfg);

/**
 * Start a DMA transfer.
 *
 * @param hdma The DMA channel number to start the transfer on.
 * @param saddr Source address of the data to transfer.
 * @param daddr Destination address to transfer the data to.
 * @param bytes Number of bytes to transfer.
 * @return 0 if successful, or an error code if failed.
 */
int sunxi_dma_start(uint32_t dma_fd, uint32_t saddr, uint32_t daddr, uint32_t bytes);

/**
 * Stop a currently running DMA transfer.
 *
 * @param hdma The DMA channel number to stop the transfer on.
 * @return 0 if successful, or an error code if failed.
 */
int sunxi_dma_stop(uint32_t dma_fd);

/**
 * Query the status of a DMA transfer.
 *
 * @param hdma The DMA channel number to query status for.
 * @return The status of the DMA transfer.
 */
int sunxi_dma_querystatus(uint32_t dma_fd);

/**
 * @brief Installs an interrupt handler for the DMA.
 *
 * This function installs a DMA interrupt handler to manage DMA transfer 
 * completion or error events.
 *
 * @param dma_fd File descriptor for the DMA device.
 * @param p Pointer to the data structure that holds the interrupt handler 
 *          context or configuration.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int sunxi_dma_install_int(uint32_t dma_fd, void *p);

/**
 * @brief Enables interrupts for the specified DMA.
 *
 * This function enables the interrupt generation for the specified DMA 
 * device, allowing it to signal when DMA operations are complete or have 
 * encountered errors.
 *
 * @param dma_fd File descriptor for the DMA device.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int sunxi_dma_enable_int(uint32_t dma_fd);

/**
 * @brief Disables interrupts for the specified DMA.
 *
 * This function disables the interrupt generation for the specified DMA 
 * device, stopping it from signaling interrupts for completed or erroneous 
 * DMA operations.
 *
 * @param dma_fd File descriptor for the DMA device.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int sunxi_dma_disable_int(uint32_t dma_fd);

/**
 * @brief Frees the interrupt resources for the specified DMA.
 *
 * This function releases any resources associated with the DMA interrupt 
 * handler that was previously installed. It should be called when the DMA 
 * is no longer in use.
 *
 * @param dma_fd File descriptor for the DMA device.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int sunxi_dma_free_int(uint32_t dma_fd);

/**
 * Perform a test DMA transfer between the specified source and destination addresses.
 *
 * @param src_addr Pointer to the source address.
 * @param dst_addr Pointer to the destination address.
 * @return 0 if successful, or an error code if failed.
 */
int sunxi_dma_test(uint32_t *src_addr, uint32_t *dst_addr, uint32_t len);

#ifdef __cplusplus
}
#endif// __cplusplus

#endif /* _SUNXI_DMA_H */
