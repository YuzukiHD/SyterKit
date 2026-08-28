/* SPDX-License-Identifier: GPL-2.0+ */

#include <common.h>
#include <driver.h>
#include <stdint.h>
#include <types.h>

#include <string.h>

#include "spif-internal.h"

/* The boot-time API is synchronous, so these buffers can be shared. */
#define SPIF_STATIC_DESC_COUNT 16U
#define SPIF_STATIC_DESC_SIZE  (SPIF_STATIC_DESC_COUNT * sizeof(struct spif_descriptor_op))
#define SPIF_STATIC_CACHE_SIZE (SPIF_MAX_TRANS_SIZE + SUNXI_SPIF_CACHELINE_SIZE)
#define SPIF_ALIGNED(x)        __attribute__((aligned(x)))

static struct spif_descriptor_op spif_static_desc[SPIF_STATIC_DESC_COUNT] SPIF_ALIGNED(SUNXI_SPIF_CACHELINE_SIZE);
static uint8_t spif_static_cache[SPIF_STATIC_CACHE_SIZE] SPIF_ALIGNED(SUNXI_SPIF_CACHELINE_SIZE);

static int spif_mem_buswidth(uint8_t buswidth)
{
	switch (buswidth) {
	case SPIF_SINGLE_MODE:
		return 0;
	case SPIF_DUAL_MODE:
		return 1;
	case SPIF_QUAD_MODE:
		return 2;
	case SPIF_OCTAL_MODE:
		return 3;
	default:
		return DRIVER_ERROR_INVALID;
	}
}

static int spif_mem_add_offset(uintptr_t base, uint32_t offset, uintptr_t *result)
{
	if (result == NULL || (uintptr_t)offset > (uintptr_t)-1 - base)
		return DRIVER_ERROR_INVALID;
	*result = base + offset;
	return 0;
}

static void spif_mem_set_common(const sunxi_spif_t *spif, struct spif_descriptor_op *desc)
{
	(void)spif;
	desc->hburst_rw_flag = sunxi_spif_platform_hburst_rw_flag();
	desc->block_data_len = sunxi_spif_platform_block_data_len();
	desc->addr_dummy_data_count = SPIF_DES_NORMAL_EN;
}

static int spif_mem_encode_phases(
	const sunxi_spif_t *spif, const struct spi_mem_op *op, struct spif_descriptor_op *desc)
{
	int width;
	int addr_size;

	(void)spif;

	if (op->cmd.nbytes != 0U) {
		if (op->cmd.nbytes != 1U || op->cmd.opcode > 0xffU)
			return DRIVER_ERROR_INVALID;
		width = spif_mem_buswidth(op->cmd.buswidth);
		if (width < 0)
			return width;
		desc->trans_phase |= SPIF_TRANS_CMD_EN;
		desc->cmd_mode_buswidth |= (uint32_t)op->cmd.opcode << SPIF_CMD_OPCODE_POS;
		desc->cmd_mode_buswidth |= (uint32_t)width <<
			sunxi_spif_platform_phase_pos(SUNXI_SPIF_PHASE_CMD);
		if (op->cmd.buswidth != SPIF_SINGLE_MODE)
			desc->cmd_mode_buswidth |= (uint32_t)width << SPIF_DATA_TRANS_POS;
	}

	if (op->addr.nbytes != 0U) {
		if (op->addr.val > 0xffffffffULL)
			return DRIVER_ERROR_INVALID;
		addr_size = sunxi_spif_platform_addr_size(op->addr.nbytes);
		if (addr_size < 0)
			return addr_size;
		width = spif_mem_buswidth(op->addr.buswidth);
		if (width < 0)
			return width;
		desc->trans_phase |= SPIF_TRANS_ADDR_EN;
		desc->flash_addr = (uint32_t)op->addr.val;
		desc->addr_dummy_data_count |= (uint32_t)addr_size;
		desc->cmd_mode_buswidth |= (uint32_t)width <<
			sunxi_spif_platform_phase_pos(SUNXI_SPIF_PHASE_ADDR);
	}

	if (op->mode.val != NULL) {
		width = spif_mem_buswidth(op->mode.buswidth);
		if (width < 0)
			return width;
		desc->trans_phase |= SPIF_TRANS_MODE_EN;
		desc->cmd_mode_buswidth |= (uint32_t)*(const uint8_t *)op->mode.val << SPIF_MODE_OPCODE_POS;
		desc->cmd_mode_buswidth |= (uint32_t)width <<
			sunxi_spif_platform_phase_pos(SUNXI_SPIF_PHASE_MODE);
	}

	if (op->dummy.nbytes != 0U) {
		desc->trans_phase |= SPIF_TRANS_DUMMY_EN;
		desc->addr_dummy_data_count |= (uint32_t)op->dummy.nbytes << SPIF_DUMMY_NUM_POS;
	}

	return 0;
}

static int spif_mem_encode_data(const sunxi_spif_t *spif, const struct spi_mem_op *op, struct spif_descriptor_op *desc,
	u32 *data_len, u32 *buffer_addr, bool *bounce, bool *bounce_rx)
{
	uintptr_t addr;
	int ret;

	(void)spif;

	*data_len = op->data.nbytes;
	*bounce = false;
	*bounce_rx = false;
	*buffer_addr = 0U;
	if (op->data.nbytes == 0U)
		return op->data.dir == SPI_MEM_NO_DATA ? 0 : DRIVER_ERROR_INVALID;
	if (op->data.dir == SPI_MEM_DATA_IN) {
		if (op->data.buf.in == NULL)
			return DRIVER_ERROR_INVALID;
		addr = (uintptr_t)op->data.buf.in;
		desc->trans_phase |= SPIF_TRANS_RX_EN;
		desc->hburst_rw_flag |= SPIF_DMA_RW_PROCESS;
		*bounce_rx = true;
	} else if (op->data.dir == SPI_MEM_DATA_OUT) {
		if (op->data.buf.out == NULL)
			return DRIVER_ERROR_INVALID;
		addr = (uintptr_t)op->data.buf.out;
		desc->trans_phase |= SPIF_TRANS_TX_EN;
		*bounce_rx = false;
	} else {
		return DRIVER_ERROR_INVALID;
	}

	if (sunxi_spif_platform_needs_short_read_bounce(op)) {
		addr = (uintptr_t)spif_static_cache;
		*bounce = true;
		memset(spif_static_cache, 0, SPIF_MIN_TRANS_NUM);
		*data_len = SPIF_MIN_TRANS_NUM;
	} else if (sunxi_spif_platform_needs_cache_bounce(addr)) {
		if (op->data.nbytes > SPIF_STATIC_CACHE_SIZE)
			return DRIVER_ERROR_INVALID;
		addr = (uintptr_t)spif_static_cache;
		*bounce = true;
		if (*bounce_rx)
			memset(spif_static_cache, 0, op->data.nbytes);
		else
			memcpy(spif_static_cache, op->data.buf.out, op->data.nbytes);
	}

	ret = sunxi_spif_platform_encode_data_addr(addr, buffer_addr);
	if (ret != 0)
		return ret;

	return 0;
}

static int spif_mem_build_descriptors(sunxi_spif_t *spif, const struct spi_mem_op *op,
	struct spif_descriptor_op **descs, u32 *transfer_len, bool *bounce, bool *bounce_rx)
{
	struct spif_descriptor_op *first = spif_static_desc;
	struct spif_descriptor_op *current;
	uint32_t max_transfer = sunxi_spif_platform_max_transfer();
	uint32_t data_len = op->data.nbytes;
	uint32_t encoded_data_addr;
	uint32_t descriptor_count;
	uint32_t descriptor_size;
	uint32_t offset;
	uint32_t index;
	uint32_t width;
	int ret;

	memset(spif_static_desc, 0, SPIF_STATIC_DESC_SIZE);
	spif_mem_set_common(spif, first);
	ret = spif_mem_encode_phases(spif, op, first);
	if (ret != 0)
		return ret;

	if (op->data.nbytes != 0U) {
		width = spif_mem_buswidth(op->data.buswidth);
		if (width < 0)
			return width;
		first->cmd_mode_buswidth |= (uint32_t)width << SPIF_DATA_TRANS_POS;
	}
	ret = spif_mem_encode_data(spif, op, first, transfer_len, &encoded_data_addr, bounce, bounce_rx);
	if (ret != 0)
		return ret;
	first->data_addr = encoded_data_addr;
	data_len = *transfer_len;
	if (op->addr.nbytes != 0U && (uint64_t)first->flash_addr + data_len > 0x100000000ULL)
		return DRIVER_ERROR_INVALID;
	if (data_len > 0U) {
		descriptor_count = (data_len - 1U) / max_transfer + 1U;
		if (descriptor_count > SPIF_STATIC_DESC_COUNT)
			return DRIVER_ERROR_INVALID;
	} else {
		descriptor_count = 1U;
	}
	descriptor_size = descriptor_count * sizeof(*first);
	if (descriptor_size > SPIF_STATIC_DESC_SIZE)
		return DRIVER_ERROR_INVALID;

	if (descriptor_count > 1U)
		memset(first + 1, 0, (descriptor_count - 1U) * sizeof(*first));

	if (data_len == 0U) {
		first->hburst_rw_flag |= SPIF_DMA_FINISH_FLAG;
		*descs = first;
		return 0;
	}

	current = first;
	offset = 0U;
	for (index = 0U; index < descriptor_count; ++index) {
		uint32_t length = data_len - offset;
		uintptr_t descriptor_address;
		uintptr_t buffer_address;
		uint32_t next_address;

		if (length > max_transfer)
			length = max_transfer;
		if (index != 0U) {
			current = first + index;
			memcpy(current, first, sizeof(*current));
			current->next_des_addr = 0U;
			if (offset > 0xffffffffU - first->flash_addr)
				return DRIVER_ERROR_INVALID;
			current->flash_addr = first->flash_addr + offset;
			if (*bounce) {
				ret = spif_mem_add_offset((uintptr_t)spif_static_cache, offset, &buffer_address);
			} else {
				buffer_address = (uintptr_t)(op->data.dir == SPI_MEM_DATA_IN ? op->data.buf.in :
											       op->data.buf.out);
				ret = spif_mem_add_offset(buffer_address, offset, &buffer_address);
			}
			if (ret == 0)
				ret = sunxi_spif_platform_encode_data_addr(buffer_address, &current->data_addr);
			if (ret != 0)
				return ret;
		}

		sunxi_spif_platform_set_data_length(current, length);
		current->hburst_rw_flag &= ~SPIF_DMA_FINISH_FLAG;

		if (index + 1U < descriptor_count) {
			descriptor_address = (uintptr_t)(current + 1);
			ret = sunxi_spif_platform_encode_desc_addr(descriptor_address, &next_address);
			if (ret != 0)
				return ret;
			current->next_des_addr = next_address;
		}
		offset += length;
	}
	current->hburst_rw_flag |= SPIF_DMA_FINISH_FLAG;
	*descs = first;
	return 0;
}

int sunxi_spif_mem_exec_op(sunxi_spif_t *spif, const struct spi_mem_op *op)
{
	struct spif_descriptor_op *descs;
	u32 transfer_len;
	bool bounce;
	bool bounce_rx;
	int ret;

	if (spif == NULL || op == NULL || !spif->initialized)
		return DRIVER_ERROR_INVALID;
	ret = spif_mem_build_descriptors(spif, op, &descs, &transfer_len, &bounce, &bounce_rx);
	if (ret != 0)
		return ret;

	ret = sunxi_spif_transfer(spif, descs, transfer_len);
	if (ret == 0 && bounce && bounce_rx && op->data.nbytes != 0U)
		memcpy(op->data.buf.in, spif_static_cache, op->data.nbytes);
	return ret;
}
