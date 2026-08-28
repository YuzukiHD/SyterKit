/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __DRIVERS_SPIF_INTERNAL_H__
#define __DRIVERS_SPIF_INTERNAL_H__

#include <stdbool.h>
#include <stdint.h>

#include <driver.h>
#include <drivers/spif/spif.h>

#include "spi-mem.h"
#include "spif-regs.h"

enum sunxi_spif_phase_field {
	SUNXI_SPIF_PHASE_MODE,
	SUNXI_SPIF_PHASE_ADDR,
	SUNXI_SPIF_PHASE_CMD,
};

struct spif_descriptor_op {
	u32 hburst_rw_flag;
	u32 block_data_len;
	u32 data_addr;
	u32 next_des_addr;
	u32 trans_phase;
	u32 flash_addr;
	u32 cmd_mode_buswidth;
	u32 addr_dummy_data_count;
};

/* Hardware revision hooks. Kconfig selects one implementation under platform/. */
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
void sunxi_spif_platform_set_data_length(struct spif_descriptor_op *desc, uint32_t length);

int sunxi_spif_mem_exec_op(sunxi_spif_t *spif, const struct spi_mem_op *op);
int sunxi_spif_transfer(sunxi_spif_t *spif, struct spif_descriptor_op *desc, uint32_t data_len);

#endif /* __DRIVERS_SPIF_INTERNAL_H__ */
