/* SPDX-License-Identifier: GPL-2.0+ */

#include <driver.h>
#include <drivers/spif/spif.h>

#include "spif-platform.h"
#include "../spif-regs.h"

uint32_t sunxi_spif_platform_max_transfer(void)
{
	return SPIF_MAX_TRANS_V1;
}

uint32_t sunxi_spif_platform_data_len_mask(void)
{
	return SPIF_DMA_DATA_LEN_V1;
}

uint32_t sunxi_spif_platform_hburst_rw_flag(void)
{
	return SPIF_DMA_HBURST_INCR16;
}

uint32_t sunxi_spif_platform_block_data_len(void)
{
	return SPIF_DMA_BLOCK_LEN_64B;
}

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

int sunxi_spif_platform_encode_data_addr(uintptr_t address, uint32_t *encoded)
{
	if (encoded == NULL || address > 0xffffffffUL || (address & 3U) != 0U)
		return DRIVER_ERROR_INVALID;
	*encoded = (uint32_t)(address >> 2);
	return 0;
}

int sunxi_spif_platform_decode_data_addr(uint32_t encoded, uintptr_t *address)
{
	if (address == NULL || encoded > 0x3fffffffU)
		return DRIVER_ERROR_INVALID;
	*address = (uintptr_t)encoded << 2;
	return 0;
}

int sunxi_spif_platform_encode_desc_addr(uintptr_t address, uint32_t *encoded)
{
	if (encoded == NULL || address > 0xffffffffUL || (address & 3U) != 0U)
		return DRIVER_ERROR_INVALID;
	*encoded = (uint32_t)(address >> 2);
	return 0;
}

bool sunxi_spif_platform_needs_short_read_bounce(const struct spi_mem_op *op)
{
	(void)op;
	return false;
}

bool sunxi_spif_platform_needs_cache_bounce(uintptr_t address)
{
	return (address % SUNXI_SPIF_CACHELINE_SIZE) != 0U;
}

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
