/* SPDX-License-Identifier: Apache-2.0 */

#ifndef __SUNXI_SPI_H__
#define __SUNXI_SPI_H__

#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <types.h>
#include <io.h>

#include "sys-gpio.h"
#include "sys-clk.h"

#include "log.h"

typedef enum {
	SPI_IO_SINGLE = 0x00,
	SPI_IO_DUAL_RX,
	SPI_IO_QUAD_RX,
	SPI_IO_QUAD_IO,
} spi_io_mode_t;

typedef struct {
	uint8_t	 mfr;
	uint16_t dev;
	uint8_t	 dlen;
} __attribute__((packed)) spi_nand_id_t;

typedef struct {
	char		 *name;
	spi_nand_id_t id;
	uint32_t	  page_size;
	uint32_t	  spare_size;
	uint32_t	  pages_per_block;
	uint32_t	  blocks_per_die;
	uint32_t	  planes_per_die;
	uint32_t	  ndies;
	spi_io_mode_t mode;
} spi_nand_info_t;

typedef struct {
	uint32_t   base;
	uint8_t	   id;
	uint32_t   clk_rate;
	gpio_mux_t gpio_cs;
	gpio_mux_t gpio_sck;
	gpio_mux_t gpio_miso;
	gpio_mux_t gpio_mosi;
	gpio_mux_t gpio_wp;
	gpio_mux_t gpio_hold;

	spi_nand_info_t info;
} sunxi_spi_t;

int		 sunxi_spi_init(sunxi_spi_t *spi);
void	 sunxi_spi_disable(sunxi_spi_t *spi);
int		 spi_nand_detect(sunxi_spi_t *spi);
uint32_t spi_nand_read(sunxi_spi_t *spi, uint8_t *buf, uint32_t addr, uint32_t rxlen);

#endif
