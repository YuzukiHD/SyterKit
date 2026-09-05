/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file spif-nor.h
 * @brief SPI NOR flash access layer for the SPIF controller.
 *
 * Provides a thin SPI NOR flash abstraction bound to the Sunxi SPIF
 * controller, covering detection, block reads and raw address reads.
 */

#ifndef __DRIVERS_MTD_SPIF_NOR_H__
#define __DRIVERS_MTD_SPIF_NOR_H__

#include <drivers/mtd/spi-nor.h>
#include <drivers/spif/spif.h>

/**
 * @struct spif_nor_t
 * @brief SPI NOR flash instance bound to a SPIF controller.
 */
typedef struct {
	int dt_node; /**< Device-tree node describing the SPI NOR flash. */
	uint8_t chip_select; /**< SPI chip-select line used by the flash. */
	uint32_t max_frequency; /**< Maximum supported SPI clock frequency, in Hz. */
	uint32_t current_frequency; /**< Currently configured SPI clock frequency, in Hz. */
	sunxi_spif_t *spif; /**< Underlying SPIF controller instance. */
	spi_nor_info_t info; /**< Detected SPI NOR flash device information. */
} spif_nor_t;

/**
 * @brief Detect and initialize the SPI NOR flash attached to a SPIF controller.
 *
 * @param[in,out] nor SPI NOR flash instance to populate.
 *
 * @return 0 on success, or DRIVER_ERROR_INVALID if no supported flash is found.
 */
int spif_nor_detect(spif_nor_t *nor);

/**
 * @brief Read one or more contiguous blocks from the SPI NOR flash.
 */
uint32_t spif_nor_read_block(spif_nor_t *nor, uint8_t *buf, uint32_t blk_no, uint32_t blk_cnt);

/**
 * @brief Read raw bytes from the SPI NOR flash starting at a given address.
 */
uint32_t spif_nor_read(spif_nor_t *nor, uint8_t *buf, uint32_t addr, uint32_t rxlen);

#endif /* __DRIVERS_MTD_SPIF_NOR_H__ */
