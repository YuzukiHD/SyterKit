/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __SYS_SPI_NAND_H__
#define __SYS_SPI_NAND_H__

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include "sys-clk.h"
#include "sys-gpio.h"
#include "sys-spi.h"

#include "log.h"

#ifdef __cplusplus
extern "C" {
#endif// __cplusplus

/**
 * @brief Represents the NAND Device ID structure.
 */
typedef struct {
	uint8_t mfr;  /**< Manufacturer ID byte. */
	uint16_t dev; /**< Device ID (16-bits) for identifying the specific NAND device. */
	uint8_t dlen; /**< Length of the ID (in bytes). */
} __attribute__((packed)) spi_nand_id_t;

/**
 * @brief Represents the specific information of a NAND Flash device.
 */
typedef struct {
	char *name;				  /**< Name of the NAND Flash device. */
	spi_nand_id_t id;		  /**< Unique identifier for the NAND Flash device, containing manufacturer and device ID. */
	uint32_t page_size;		  /**< Size of the data page (in bytes). */
	uint32_t spare_size;	  /**< Size of the spare area for additional information (in bytes). */
	uint32_t pages_per_block; /**< Number of pages contained in a single block. */
	uint32_t blocks_per_die;  /**< Number of blocks present on a single die. */
	uint32_t planes_per_die;  /**< Number of planes present on a single die. */
	uint32_t ndies;			  /**< Total number of dies in the NAND package. */
	spi_io_mode_t mode;		  /**< I/O mode used for communication (assumes the existence of a spi_io_mode_t type). */
} spi_nand_info_t;

/**
 * Detect and initialize SPI NAND flash.
 *
 * @param spi Pointer to the sunxi_spi_t structure.
 * @return 0 on success, -1 on failure.
 */
int spi_nand_detect(sunxi_spi_t *spi);

/**
 * Read data from SPI NAND flash.
 *
 * @param spi Pointer to the sunxi_spi_t structure.
 * @param buf Pointer to the buffer to store the read data.
 * @param addr Starting address to read from.
 * @param rxlen Number of bytes to read.
 * @return Number of bytes read on success, -1 on failure.
 */
uint32_t spi_nand_read(sunxi_spi_t *spi, uint8_t *buf, uint32_t addr, uint32_t rxlen);

#ifdef __cplusplus
}
#endif// __cplusplus

#endif// __SYS_SPI_NAND_H__