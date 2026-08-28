/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __DRIVERS_MTD_SPIF_NOR_H__
#define __DRIVERS_MTD_SPIF_NOR_H__

#include <drivers/mtd/spi-nor.h>
#include <drivers/spif/spif.h>

typedef struct {
	int dt_node;
	uint8_t chip_select;
	uint32_t max_frequency;
	uint32_t current_frequency;
	sunxi_spif_t *spif;
	spi_nor_info_t info;
} spif_nor_t;

int spif_nor_detect(spif_nor_t *nor);
uint32_t spif_nor_read_block(spif_nor_t *nor, uint8_t *buf, uint32_t blk_no, uint32_t blk_cnt);
uint32_t spif_nor_read(spif_nor_t *nor, uint8_t *buf, uint32_t addr, uint32_t rxlen);

#endif /* __DRIVERS_MTD_SPIF_NOR_H__ */
