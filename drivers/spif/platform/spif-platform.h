/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __DRIVERS_SPIF_PLATFORM_H__
#define __DRIVERS_SPIF_PLATFORM_H__

#include <stdbool.h>
#include <stdint.h>

#include <drivers/spif/spif.h>

enum sunxi_spif_phase_field {
	SUNXI_SPIF_PHASE_MODE,
	SUNXI_SPIF_PHASE_ADDR,
	SUNXI_SPIF_PHASE_CMD,
};

uint32_t sunxi_spif_platform_max_transfer(void);
uint32_t sunxi_spif_platform_data_len_mask(void);
uint32_t sunxi_spif_platform_hburst_rw_flag(void);
uint32_t sunxi_spif_platform_block_data_len(void);
uint32_t sunxi_spif_platform_phase_pos(enum sunxi_spif_phase_field field);
int sunxi_spif_platform_addr_size(uint8_t nbytes);
int sunxi_spif_platform_encode_data_addr(uintptr_t address, uint32_t *encoded);
int sunxi_spif_platform_decode_data_addr(uint32_t encoded, uintptr_t *address);
int sunxi_spif_platform_encode_desc_addr(uintptr_t address, uint32_t *encoded);
bool sunxi_spif_platform_needs_short_read_bounce(const struct spi_mem_op *op);
bool sunxi_spif_platform_needs_cache_bounce(uintptr_t address);
void sunxi_spif_platform_set_data_length(uint32_t *block_data_len, uint32_t *addr_dummy_data_count, uint32_t length);

#endif /* __DRIVERS_SPIF_PLATFORM_H__ */
