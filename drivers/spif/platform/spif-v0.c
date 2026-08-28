/* SPDX-License-Identifier: GPL-2.0+ */

#include "../spif-internal.h"

uint32_t sunxi_spif_platform_max_transfer(void)
{
	return SPIF_MAX_TRANS_V0;
}

uint32_t sunxi_spif_platform_data_len_mask(void)
{
	return SPIF_DMA_DATA_LEN_V0;
}

uint32_t sunxi_spif_platform_hburst_rw_flag(void)
{
	return SPIF_DMA_HBURST_INCR4;
}

uint32_t sunxi_spif_platform_block_data_len(void)
{
	return 1U << 16;
}

uint32_t sunxi_spif_platform_phase_pos(enum sunxi_spif_phase_field field)
{
	switch (field) {
	case SUNXI_SPIF_PHASE_MODE:
		return SPIF_MODE_TRANS_POS_V0;
	case SUNXI_SPIF_PHASE_ADDR:
		return SPIF_ADDR_TRANS_POS_V0;
	case SUNXI_SPIF_PHASE_CMD:
		return SPIF_CMD_TRANS_POS_V0;
	default:
		return 0U;
	}
}

int sunxi_spif_platform_addr_size(uint8_t nbytes)
{
	switch (nbytes) {
	case 3U:
		return SPIF_ADDR_SIZE_24BIT;
	case 4U:
		return SPIF_ADDR_SIZE_32BIT;
	default:
		return DRIVER_ERROR_INVALID;
	}
}

int sunxi_spif_platform_encode_data_addr(uintptr_t address, uint32_t *encoded)
{
	if (encoded == NULL || address > 0xffffffffUL)
		return DRIVER_ERROR_INVALID;
	*encoded = (uint32_t)address;
	return 0;
}

int sunxi_spif_platform_decode_data_addr(uint32_t encoded, uintptr_t *address)
{
	if (address == NULL)
		return DRIVER_ERROR_INVALID;
	*address = (uintptr_t)encoded;
	return 0;
}

int sunxi_spif_platform_encode_desc_addr(uintptr_t address, uint32_t *encoded)
{
	if (encoded == NULL || address > 0xffffffffUL)
		return DRIVER_ERROR_INVALID;
	*encoded = (uint32_t)address;
	return 0;
}

bool sunxi_spif_platform_needs_short_read_bounce(const struct spi_mem_op *op)
{
	return op != NULL && op->data.dir == SPI_MEM_DATA_IN && op->data.nbytes < SPIF_MIN_TRANS_NUM;
}

bool sunxi_spif_platform_needs_cache_bounce(uintptr_t address)
{
	(void)address;
	return false;
}

void sunxi_spif_platform_set_data_length(struct spif_descriptor_op *desc, uint32_t length)
{
	desc->block_data_len = (desc->block_data_len & ~SPIF_DMA_DATA_LEN_V0) |
		(length & SPIF_DMA_DATA_LEN_V0);
	desc->addr_dummy_data_count = (desc->addr_dummy_data_count & ~SPIF_DMA_DATA_LEN_V0) |
		(length & SPIF_DMA_DATA_LEN_V0);
}
