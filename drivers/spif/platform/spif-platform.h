/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file spif-platform.h
 * @brief SPIF platform abstraction interface.
 *
 * Declares the platform-dependent SPIF helpers that differ between the
 * SPIF v0, v1 and v2 hardware implementations.
 */

#ifndef __DRIVERS_SPIF_PLATFORM_H__
#define __DRIVERS_SPIF_PLATFORM_H__

#include <stdbool.h>
#include <stdint.h>

#include <drivers/spif/spif.h>

/**
 * @enum sunxi_spif_phase_field
 * @brief SPI phase type used to look up a phase bit position.
 */
enum sunxi_spif_phase_field {
	SUNXI_SPIF_PHASE_MODE, /**< SPI mode phase. */
	SUNXI_SPIF_PHASE_ADDR, /**< Address phase. */
	SUNXI_SPIF_PHASE_CMD, /**< Command phase. */
};

/**
 * @brief Return the maximum DMA transfer length supported by the platform.
 *
 * @return Maximum transfer length, in bytes.
 */
uint32_t sunxi_spif_platform_max_transfer(void);

/**
 * @brief Return the DMA data-length field mask.
 *
 * @return Bitmask covering the data-length field in the DMA register.
 */
uint32_t sunxi_spif_platform_data_len_mask(void);

/**
 * @brief Return the DMA burst flag used for read/write bursts.
 *
 * @return Hardware burst flag value.
 */
uint32_t sunxi_spif_platform_hburst_rw_flag(void);

/**
 * @brief Return the DMA block data length value.
 *
 * @return Block data length register value.
 */
uint32_t sunxi_spif_platform_block_data_len(void);

/**
 * @brief Return the bit position of a given SPI phase field.
 *
 * @param[in] field Phase field to look up.
 *
 * @return The bit position of the phase field, or 0 for an unknown field.
 */
uint32_t sunxi_spif_platform_phase_pos(enum sunxi_spif_phase_field field);

/**
 * @brief Map a SPI address width to the hardware address-size encoding.
 *
 * @param[in] nbytes Address width in bytes (1, 2, 3 or 4).
 *
 * @return The hardware address-size value, or DRIVER_ERROR_INVALID.
 */
int sunxi_spif_platform_addr_size(uint8_t nbytes);

/**
 * @brief Encode a CPU address for the DMA data register.
 *
 * @param[in] address CPU address to encode.
 * @param[out] encoded Buffer for the encoded DMA address.
 *
 * @return 0 on success, or DRIVER_ERROR_INVALID on failure.
 */
int sunxi_spif_platform_encode_data_addr(uintptr_t address, uint32_t *encoded);

/**
 * @brief Decode a DMA data-register address back into a CPU address.
 *
 * @param[in] encoded Encoded DMA address.
 * @param[out] address Buffer for the decoded CPU address.
 *
 * @return 0 on success, or DRIVER_ERROR_INVALID on failure.
 */
int sunxi_spif_platform_decode_data_addr(uint32_t encoded, uintptr_t *address);

/**
 * @brief Encode a CPU address for the DMA descriptor register.
 *
 * @param[in] address CPU address to encode.
 * @param[out] encoded Buffer for the encoded descriptor address.
 *
 * @return 0 on success, or DRIVER_ERROR_INVALID on failure.
 */
int sunxi_spif_platform_encode_desc_addr(uintptr_t address, uint32_t *encoded);

/**
 * @brief Report whether a short read operation needs a bounce buffer.
 *
 * @param[in] op SPI memory operation to check.
 *
 * @return true if the operation needs a bounce buffer, false otherwise.
 */
bool sunxi_spif_platform_needs_short_read_bounce(const struct spi_mem_op *op);

/**
 * @brief Report whether a cache bounce is required for an address.
 *
 * @param[in] address CPU address to check.
 *
 * @return true if a cache bounce is required, false otherwise.
 */
bool sunxi_spif_platform_needs_cache_bounce(uintptr_t address);

/**
 * @brief Update the DMA block length and transfer count for a data length.
 *
 * @param[in,out] block_data_len DMA block data-length register value.
 * @param[in,out] addr_dummy_data_count DMA address/dummy/data count register value.
 * @param[in] length Transfer length, in bytes.
 */
void sunxi_spif_platform_set_data_length(uint32_t *block_data_len, uint32_t *addr_dummy_data_count, uint32_t length);

#endif /* __DRIVERS_SPIF_PLATFORM_H__ */
