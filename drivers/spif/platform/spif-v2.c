/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file spif-v2.c
 * @brief SPIF platform helpers for the v2 SPIF hardware.
 */

#include <driver.h>
#include <drivers/spif/spif.h>

#include "spif-platform.h"
#include "../spif-regs.h"

/**
 * @brief Return the maximum DMA transfer length supported by the platform.
 *
 * @return Maximum transfer length, in bytes.
 */
uint32_t sunxi_spif_platform_max_transfer(void)
{
	return SPIF_MAX_TRANS_V1;
}

/**
 * @brief Return the DMA data-length field mask.
 *
 * @return Bitmask covering the data-length field in the DMA register.
 */
uint32_t sunxi_spif_platform_data_len_mask(void)
{
	return SPIF_DMA_DATA_LEN_V1;
}

/**
 * @brief Return the DMA burst flag used for read/write bursts.
 *
 * @return Hardware burst flag value.
 */
uint32_t sunxi_spif_platform_hburst_rw_flag(void)
{
	return SPIF_DMA_HBURST_INCR16;
}

/**
 * @brief Return the DMA block data length value.
 *
 * @return Block data length register value.
 */
uint32_t sunxi_spif_platform_block_data_len(void)
{
	return SPIF_DMA_BLOCK_LEN_64B;
}

/**
 * @brief Return the bit position of a given SPI phase field.
 *
 * @param[in] field Phase field to look up.
 *
 * @return The bit position of the phase field, or 0 for an unknown field.
 */
uint32_t sunxi_spif_platform_phase_pos(enum sunxi_spif_phase_field field)
{
	switch (field) {
	case SUNXI_SPIF_PHASE_MODE:
		return SPIF_MODE_TRANS_POS_V1;
	case SUNXI_SPIF_PHASE_ADDR:
		return SPIF_ADDR_TRANS_POS_V1;
	case SUNXI_SPIF_PHASE_CMD:
		return SPIF_CMD_TRANS_POS_V1;
	default:
		return 0U;
	}
}

/**
 * @brief Map a SPI address width to the hardware address-size encoding.
 *
 * @param[in] nbytes Address width in bytes (1, 2, 3 or 4).
 *
 * @return The hardware address-size value, or DRIVER_ERROR_INVALID.
 */
int sunxi_spif_platform_addr_size(uint8_t nbytes)
{
	switch (nbytes) {
	case 1U:
		return SPIF_ADDR_SIZE_8BIT_V2;
	case 2U:
		return SPIF_ADDR_SIZE_16BIT_V2;
	case 3U:
		return SPIF_ADDR_SIZE_24BIT_V2;
	case 4U:
		return SPIF_ADDR_SIZE_32BIT_V2;
	default:
		return DRIVER_ERROR_INVALID;
	}
}

/**
 * @brief Encode a CPU address for the DMA data register.
 *
 * @param[in] address CPU address to encode.
 * @param[out] encoded Buffer for the encoded DMA address.
 *
 * @return 0 on success, or DRIVER_ERROR_INVALID on failure.
 */
int sunxi_spif_platform_encode_data_addr(uintptr_t address, uint32_t *encoded)
{
	if (encoded == NULL || address > 0xffffffffUL || (address & 3U) != 0U)
		return DRIVER_ERROR_INVALID;
	*encoded = (uint32_t)(address >> 2);
	return 0;
}

/**
 * @brief Decode a DMA data-register address back into a CPU address.
 *
 * @param[in] encoded Encoded DMA address.
 * @param[out] address Buffer for the decoded CPU address.
 *
 * @return 0 on success, or DRIVER_ERROR_INVALID on failure.
 */
int sunxi_spif_platform_decode_data_addr(uint32_t encoded, uintptr_t *address)
{
	if (address == NULL || encoded > 0x3fffffffU)
		return DRIVER_ERROR_INVALID;
	*address = (uintptr_t)encoded << 2;
	return 0;
}

/**
 * @brief Encode a CPU address for the DMA descriptor register.
 *
 * @param[in] address CPU address to encode.
 * @param[out] encoded Buffer for the encoded descriptor address.
 *
 * @return 0 on success, or DRIVER_ERROR_INVALID on failure.
 */
int sunxi_spif_platform_encode_desc_addr(uintptr_t address, uint32_t *encoded)
{
	if (encoded == NULL || address > 0xffffffffUL || (address & 3U) != 0U)
		return DRIVER_ERROR_INVALID;
	*encoded = (uint32_t)(address >> 2);
	return 0;
}

/**
 * @brief Report whether a short read operation needs a bounce buffer.
 *
 * @param[in] op SPI memory operation to check.
 *
 * @return true if the operation needs a bounce buffer, false otherwise.
 */
bool sunxi_spif_platform_needs_short_read_bounce(const struct spi_mem_op *op)
{
	(void)op;
	return false;
}

/**
 * @brief Report whether a cache bounce is required for an address.
 *
 * @param[in] address CPU address to check.
 *
 * @return true if a cache bounce is required, false otherwise.
 */
bool sunxi_spif_platform_needs_cache_bounce(uintptr_t address)
{
	return (address % SUNXI_SPIF_CACHELINE_SIZE) != 0U;
}

/**
 * @brief Update the DMA block length and transfer count for a data length.
 *
 * @param[in,out] block_data_len DMA block data-length register value.
 * @param[in,out] addr_dummy_data_count DMA address/dummy/data count register value.
 * @param[in] length Transfer length, in bytes.
 */
void sunxi_spif_platform_set_data_length(uint32_t *block_data_len, uint32_t *addr_dummy_data_count, uint32_t length)
{
	*block_data_len = (*block_data_len & ~SPIF_DMA_DATA_LEN_V1) |
		(length & SPIF_DMA_DATA_LEN_V1);
	*addr_dummy_data_count &= ~(SPIF_DMA_TRANS_NUM_16BIT | SPIF_DMA_TRANS_NUM);
	if (length == SPIF_MAX_TRANS_V1)
		*addr_dummy_data_count |= SPIF_DMA_TRANS_NUM_16BIT;
	else
		*addr_dummy_data_count |= length;
}
