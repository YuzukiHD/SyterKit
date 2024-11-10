/* SPDX-License-Identifier: Apache-2.0 */

#ifndef __SYS_SPI_NAND_H__
#define __SYS_SPI_NAND_H__

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include "sys-clk.h"
#include "sys-spi.h"
#include "sys-gpio.h"

#include "log.h"

#ifdef __cplusplus
extern "C" {
#endif// __cplusplus

typedef struct {
    uint8_t mfr;
    uint16_t dev;
    uint8_t dlen;
} __attribute__((packed)) spi_nand_id_t;

typedef struct {
    char *name;
    spi_nand_id_t id;
    uint32_t page_size;
    uint32_t spare_size;
    uint32_t pages_per_block;
    uint32_t blocks_per_die;
    uint32_t planes_per_die;
    uint32_t ndies;
    spi_io_mode_t mode;
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
#endif // __cplusplus

#endif // __SYS_SPI_NAND_H__