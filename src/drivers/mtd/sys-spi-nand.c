/* SPDX-License-Identifier: GPL-2.0+ */

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <timer.h>

#include <log.h>

#include <sys-clk.h>
#include <sys-dma.h>
#include <sys-gpio.h>

#include "sys-spi-nand.h"
#include "sys-spi.h"

enum {
	OPCODE_READ_ID = 0x9f,
	OPCODE_READ_STATUS = 0x0f,
	OPCODE_WRITE_STATUS = 0x1f,
	OPCODE_READ_PAGE = 0x13,
	OPCODE_READ = 0x03,
	OPCODE_FAST_READ = 0x0b,
	OPCODE_FAST_READ_DUAL_O = 0x3b,
	OPCODE_FAST_READ_QUAD_O = 0x6b,
	OPCODE_FAST_READ_DUAL_IO = 0xbb,
	OPCODE_FAST_READ_QUAD_IO = 0xeb,
	OPCODE_WRITE_ENABLE = 0x06,
	OPCODE_BLOCK_ERASE = 0xd8,
	OPCODE_PROGRAM_LOAD = 0x02,
	OPCODE_PROGRAM_EXEC = 0x10,
	OPCODE_RESET = 0xff,
};

/* Micron calls it "feature", Winbond "status".
   We'll call it config for simplicity. */
enum {
	CONFIG_ADDR_PROTECT = 0xa0,
	CONFIG_ADDR_OTP = 0xb0,
	CONFIG_ADDR_STATUS = 0xc0,
	CONFIG_POS_BUF = 0x08,// Micron specific
};

typedef enum {
	SPI_NAND_MFR_WINBOND = 0xef,
	SPI_NAND_MFR_GIGADEVICE = 0xc8,
	SPI_NAND_MFR_MACRONIX = 0xc2,
	SPI_NAND_MFR_MICRON = 0x2c,
	SPI_NAND_MFR_FORESEE = 0xcd,
	SPI_NAND_MFR_ETRON = 0xd5,
	SPI_NAND_MFR_XTX = 0x0b,
} spi_mfr_id;

static const spi_nand_info_t spi_nand_infos[] = {
		/* Winbond */
		{"W25N512GV", {.mfr = SPI_NAND_MFR_WINBOND, .dev = 0xaa20, 2}, 2048, 64, 64, 512, 1, 1, SPI_IO_QUAD_RX},
		{"W25N01GV", {.mfr = SPI_NAND_MFR_WINBOND, .dev = 0xaa21, 2}, 2048, 64, 64, 1024, 1, 1, SPI_IO_QUAD_RX},
		{"W25M02GV", {.mfr = SPI_NAND_MFR_WINBOND, .dev = 0xab21, 2}, 2048, 64, 64, 1024, 1, 2, SPI_IO_QUAD_RX},
		{"W25N02KV", {.mfr = SPI_NAND_MFR_WINBOND, .dev = 0xaa22, 2}, 2048, 128, 64, 2048, 1, 1, SPI_IO_QUAD_RX},

		/* Gigadevice */
		{"GD5F1GQ4UAWxx", {.mfr = SPI_NAND_MFR_GIGADEVICE, .dev = 0x10, 1}, 2048, 64, 64, 1024, 1, 1, SPI_IO_QUAD_RX},
		{"GD5F1GQ5UExxG", {.mfr = SPI_NAND_MFR_GIGADEVICE, .dev = 0x51, 1}, 2048, 128, 64, 1024, 1, 1, SPI_IO_QUAD_RX},
		{"GD5F1GQ4UExIG", {.mfr = SPI_NAND_MFR_GIGADEVICE, .dev = 0xd1, 1}, 2048, 128, 64, 1024, 1, 1, SPI_IO_QUAD_RX},
		{"GD5F2GQ4xFxxG", {.mfr = SPI_NAND_MFR_GIGADEVICE, .dev = 0xd2, 1}, 2048, 256, 64, 2048, 1, 1, SPI_IO_QUAD_RX},
		{"GD5F1GQ4UExxH", {.mfr = SPI_NAND_MFR_GIGADEVICE, .dev = 0xd9, 1}, 2048, 64, 64, 1024, 1, 1, SPI_IO_QUAD_RX},
		{"GD5F1GQ4xAYIG", {.mfr = SPI_NAND_MFR_GIGADEVICE, .dev = 0xf1, 1}, 2048, 64, 64, 1024, 1, 1, SPI_IO_QUAD_RX},
		{"GD5F2GQ4UExIG", {.mfr = SPI_NAND_MFR_GIGADEVICE, .dev = 0xd2, 1}, 2048, 128, 64, 2048, 1, 1, SPI_IO_QUAD_RX},
		{"GD5F2GQ5UExxH", {.mfr = SPI_NAND_MFR_GIGADEVICE, .dev = 0x32, 1}, 2048, 64, 64, 2048, 1, 1, SPI_IO_QUAD_RX},
		{"GD5F2GQ4xAYIG", {.mfr = SPI_NAND_MFR_GIGADEVICE, .dev = 0xf2, 1}, 2048, 64, 64, 2048, 1, 1, SPI_IO_QUAD_RX},
		{"GD5F4GQ4UBxIG", {.mfr = SPI_NAND_MFR_GIGADEVICE, .dev = 0xd4, 1}, 4096, 256, 64, 2048, 1, 1, SPI_IO_QUAD_RX},
		{"GD5F4GQ4xAYIG", {.mfr = SPI_NAND_MFR_GIGADEVICE, .dev = 0xf4, 1}, 2048, 64, 64, 4096, 1, 1, SPI_IO_QUAD_RX},
		{"GD5F2GQ5UExxG", {.mfr = SPI_NAND_MFR_GIGADEVICE, .dev = 0x52, 1}, 2048, 128, 64, 2048, 1, 1, SPI_IO_QUAD_RX},
		{"GD5F4GQ4UCxIG", {.mfr = SPI_NAND_MFR_GIGADEVICE, .dev = 0xb4, 1}, 4096, 256, 64, 2048, 1, 1, SPI_IO_QUAD_RX},
		{"GD5F4GQ4RCxIG", {.mfr = SPI_NAND_MFR_GIGADEVICE, .dev = 0xa4, 1}, 4096, 256, 64, 2048, 1, 1, SPI_IO_QUAD_RX},

		/* Macronix */
		{"MX35LF1GE4AB", {.mfr = SPI_NAND_MFR_MACRONIX, .dev = 0x12, 1}, 2048, 64, 64, 1024, 1, 1, SPI_IO_DUAL_RX},
		{"MX35LF1G24AD", {.mfr = SPI_NAND_MFR_MACRONIX, .dev = 0x14, 1}, 2048, 128, 64, 1024, 1, 1, SPI_IO_DUAL_RX},
		{"MX31LF1GE4BC", {.mfr = SPI_NAND_MFR_MACRONIX, .dev = 0x1e, 1}, 2048, 64, 64, 1024, 1, 1, SPI_IO_DUAL_RX},
		{"MX35LF2GE4AB", {.mfr = SPI_NAND_MFR_MACRONIX, .dev = 0x22, 1}, 2048, 64, 64, 2048, 1, 1, SPI_IO_DUAL_RX},
		{"MX35LF2G24AD", {.mfr = SPI_NAND_MFR_MACRONIX, .dev = 0x24, 1}, 2048, 128, 64, 2048, 1, 1, SPI_IO_DUAL_RX},
		{"MX35LF2GE4AD", {.mfr = SPI_NAND_MFR_MACRONIX, .dev = 0x26, 1}, 2048, 128, 64, 2048, 1, 1, SPI_IO_DUAL_RX},
		{"MX35LF2G14AC", {.mfr = SPI_NAND_MFR_MACRONIX, .dev = 0x20, 1}, 2048, 64, 64, 2048, 1, 1, SPI_IO_DUAL_RX},
		{"MX35LF4G24AD", {.mfr = SPI_NAND_MFR_MACRONIX, .dev = 0x35, 1}, 4096, 256, 64, 2048, 1, 1, SPI_IO_DUAL_RX},
		{"MX35LF4GE4AD", {.mfr = SPI_NAND_MFR_MACRONIX, .dev = 0x37, 1}, 4096, 256, 64, 2048, 1, 1, SPI_IO_DUAL_RX},

		/* Etron */
		{"EM73B044VCA", {.mfr = SPI_NAND_MFR_ETRON, .dev = 0x01, 1}, 2048, 64, 64, 512, 1, 1, SPI_IO_QUAD_RX},
		{"EM73C044SNB", {.mfr = SPI_NAND_MFR_ETRON, .dev = 0x11, 1}, 2048, 120, 64, 1024, 1, 1, SPI_IO_QUAD_RX},
		{"EM73C044SNF", {.mfr = SPI_NAND_MFR_ETRON, .dev = 0x09, 1}, 2048, 128, 64, 1024, 1, 1, SPI_IO_QUAD_RX},
		{"EM73C044VCA", {.mfr = SPI_NAND_MFR_ETRON, .dev = 0x18, 1}, 2048, 64, 64, 1024, 1, 1, SPI_IO_QUAD_RX},
		{"EM73C044SNA", {.mfr = SPI_NAND_MFR_ETRON, .dev = 0x19, 1}, 2048, 64, 128, 512, 1, 1, SPI_IO_QUAD_RX},
		{"EM73C044VCD", {.mfr = SPI_NAND_MFR_ETRON, .dev = 0x1c, 1}, 2048, 64, 64, 1024, 1, 1, SPI_IO_QUAD_RX},
		{"EM73C044SND", {.mfr = SPI_NAND_MFR_ETRON, .dev = 0x1d, 1}, 2048, 64, 64, 1024, 1, 1, SPI_IO_QUAD_RX},
		{"EM73D044SND", {.mfr = SPI_NAND_MFR_ETRON, .dev = 0x1e, 1}, 2048, 64, 64, 2048, 1, 1, SPI_IO_QUAD_RX},
		{"EM73C044VCC", {.mfr = SPI_NAND_MFR_ETRON, .dev = 0x22, 1}, 2048, 64, 64, 1024, 1, 1, SPI_IO_QUAD_RX},
		{"EM73C044VCF", {.mfr = SPI_NAND_MFR_ETRON, .dev = 0x25, 1}, 2048, 64, 64, 1024, 1, 1, SPI_IO_QUAD_RX},
		{"EM73C044SNC", {.mfr = SPI_NAND_MFR_ETRON, .dev = 0x31, 1}, 2048, 128, 64, 1024, 1, 1, SPI_IO_QUAD_RX},
		{"EM73D044SNC", {.mfr = SPI_NAND_MFR_ETRON, .dev = 0x0a, 1}, 2048, 120, 64, 2048, 1, 1, SPI_IO_QUAD_RX},
		{"EM73D044SNA", {.mfr = SPI_NAND_MFR_ETRON, .dev = 0x12, 1}, 2048, 128, 64, 2048, 1, 1, SPI_IO_QUAD_RX},
		{"EM73D044SNF", {.mfr = SPI_NAND_MFR_ETRON, .dev = 0x10, 1}, 2048, 128, 64, 2048, 1, 1, SPI_IO_QUAD_RX},
		{"EM73D044VCA", {.mfr = SPI_NAND_MFR_ETRON, .dev = 0x13, 1}, 2048, 128, 64, 2048, 1, 1, SPI_IO_QUAD_RX},
		{"EM73D044VCB", {.mfr = SPI_NAND_MFR_ETRON, .dev = 0x14, 1}, 2048, 64, 64, 2048, 1, 1, SPI_IO_QUAD_RX},
		{"EM73D044VCD", {.mfr = SPI_NAND_MFR_ETRON, .dev = 0x17, 1}, 2048, 128, 64, 2048, 1, 1, SPI_IO_QUAD_RX},
		{"EM73D044VCH", {.mfr = SPI_NAND_MFR_ETRON, .dev = 0x1b, 1}, 2048, 64, 64, 2048, 1, 1, SPI_IO_QUAD_RX},
		{"EM73D044SND", {.mfr = SPI_NAND_MFR_ETRON, .dev = 0x1d, 1}, 2048, 64, 64, 2048, 1, 1, SPI_IO_QUAD_RX},
		{"EM73D044VCG", {.mfr = SPI_NAND_MFR_ETRON, .dev = 0x1f, 1}, 2048, 64, 64, 2048, 1, 1, SPI_IO_QUAD_RX},
		{"EM73D044VCE", {.mfr = SPI_NAND_MFR_ETRON, .dev = 0x20, 1}, 2048, 64, 64, 2048, 1, 1, SPI_IO_QUAD_RX},
		{"EM73D044VCL", {.mfr = SPI_NAND_MFR_ETRON, .dev = 0x2e, 1}, 2048, 128, 64, 2048, 1, 1, SPI_IO_QUAD_RX},
		{"EM73D044SNB", {.mfr = SPI_NAND_MFR_ETRON, .dev = 0x32, 1}, 2048, 128, 64, 2048, 1, 1, SPI_IO_QUAD_RX},
		{"EM73E044SNA", {.mfr = SPI_NAND_MFR_ETRON, .dev = 0x03, 1}, 4096, 256, 64, 2048, 1, 1, SPI_IO_QUAD_RX},
		{"EM73E044SND", {.mfr = SPI_NAND_MFR_ETRON, .dev = 0x0b, 1}, 4096, 240, 64, 2048, 1, 1, SPI_IO_QUAD_RX},
		{"EM73E044SNB", {.mfr = SPI_NAND_MFR_ETRON, .dev = 0x23, 1}, 4096, 256, 64, 2048, 1, 1, SPI_IO_QUAD_RX},
		{"EM73E044VCA", {.mfr = SPI_NAND_MFR_ETRON, .dev = 0x2c, 1}, 4096, 256, 64, 2048, 1, 1, SPI_IO_QUAD_RX},
		{"EM73E044VCB", {.mfr = SPI_NAND_MFR_ETRON, .dev = 0x2f, 1}, 2048, 128, 64, 4096, 1, 1, SPI_IO_QUAD_RX},
		{"EM73F044SNA", {.mfr = SPI_NAND_MFR_ETRON, .dev = 0x24, 1}, 4096, 256, 64, 4096, 1, 1, SPI_IO_QUAD_RX},
		{"EM73F044VCA", {.mfr = SPI_NAND_MFR_ETRON, .dev = 0x2d, 1}, 4096, 256, 64, 4096, 1, 1, SPI_IO_QUAD_RX},
		{"EM73E044SNE", {.mfr = SPI_NAND_MFR_ETRON, .dev = 0x0e, 1}, 4096, 256, 64, 4096, 1, 1, SPI_IO_QUAD_RX},
		{"EM73C044SNG", {.mfr = SPI_NAND_MFR_ETRON, .dev = 0x0c, 1}, 2048, 120, 64, 1024, 1, 1, SPI_IO_QUAD_RX},
		{"EM73D044VCN", {.mfr = SPI_NAND_MFR_ETRON, .dev = 0x0f, 1}, 2048, 64, 64, 2048, 1, 1, SPI_IO_QUAD_RX},

		/* Micron */
		{"MT29F1G01AAADD", {.mfr = SPI_NAND_MFR_MICRON, .dev = 0x12, 1}, 2048, 64, 64, 1024, 1, 1, SPI_IO_DUAL_RX},
		{"MT29F1G01ABAFD", {.mfr = SPI_NAND_MFR_MICRON, .dev = 0x14, 1}, 2048, 128, 64, 1024, 1, 1, SPI_IO_DUAL_RX},
		{"MT29F2G01AAAED", {.mfr = SPI_NAND_MFR_MICRON, .dev = 0x9f, 1}, 2048, 64, 64, 2048, 2, 1, SPI_IO_DUAL_RX},
		{"MT29F2G01ABAGD", {.mfr = SPI_NAND_MFR_MICRON, .dev = 0x24, 1}, 2048, 128, 64, 2048, 2, 1, SPI_IO_DUAL_RX},
		{"MT29F4G01AAADD", {.mfr = SPI_NAND_MFR_MICRON, .dev = 0x32, 1}, 2048, 64, 64, 4096, 2, 1, SPI_IO_DUAL_RX},
		{"MT29F4G01ABAFD", {.mfr = SPI_NAND_MFR_MICRON, .dev = 0x34, 1}, 4096, 256, 64, 2048, 1, 1, SPI_IO_DUAL_RX},
		{"MT29F4G01ADAGD", {.mfr = SPI_NAND_MFR_MICRON, .dev = 0x36, 1}, 2048, 128, 64, 2048, 2, 2, SPI_IO_DUAL_RX},
		{"MT29F8G01ADAFD", {.mfr = SPI_NAND_MFR_MICRON, .dev = 0x46, 1}, 4096, 256, 64, 2048, 1, 2, SPI_IO_DUAL_RX},

		/* FORESEE */
		{"FS35SQA001G", {.mfr = SPI_NAND_MFR_FORESEE, .dev = 0x7171, 2}, 2048, 64, 64, 1024, 1, 1, SPI_IO_QUAD_RX},
		{"FS35ND01G", {.mfr = SPI_NAND_MFR_FORESEE, .dev = 0xb1cd, 2}, 2048, 128, 64, 1024, 1, 1, SPI_IO_QUAD_RX},

		/* XTX */
		{"XT26G01C", {.mfr = SPI_NAND_MFR_XTX, .dev = 0x11, 1}, 2048, 128, 64, 1024, 1, 1, SPI_IO_QUAD_RX},
};

static spi_nand_info_t info; /* Static variable to store SPI NAND information */

/**
 * Retrieve SPI NAND information.
 *
 * @param spi Pointer to the sunxi_spi_t structure.
 * @return 0 on success, -1 on failure.
 */
static int spi_nand_info(sunxi_spi_t *spi) {
	spi_nand_info_t *info_table; /* Pointer to the SPI NAND information table */
	spi_nand_id_t id;			 /* Structure to store the SPI NAND ID */
	uint8_t tx[2];				 /* Transmit buffer */
	uint8_t rx[5], *rxp;		 /* Receive buffer and pointer */
	int i, r;					 /* Loop counter and return value */

	tx[0] = OPCODE_READ_ID; /* Command to read SPI NAND ID */
	tx[1] = 0x0;
	r = sunxi_spi_transfer(spi, SPI_IO_SINGLE, tx, 1, rx, 5); /* Perform SPI transfer */
	if (r < 0)
		return r;

	printk_debug("rx: 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x\n", rx[0], rx[1], rx[2], rx[3], rx[4]);

	/* Check if the first byte of the received data is 0xff */
	if (rx[0] == 0xff) {
		rxp = rx + 1; /* If yes, shift the receive pointer by one byte (dummy data) */
	} else {
		rxp = rx;
	}

	id.mfr = rxp[0]; /* Set the manufacturer ID */
	for (i = 0; i < ARRAY_SIZE(spi_nand_infos); i++) {
		info_table = (spi_nand_info_t *) &spi_nand_infos[i]; /* Get a pointer to the current info table entry */
		if (info_table->id.dlen == 2) {
			id.dev = (((uint16_t) rxp[1]) << 8 | rxp[2]); /* Set the device ID (16-bit) */
		} else {
			id.dev = rxp[1]; /* Set the device ID (8-bit) */
		}
		/* Check if the manufacturer ID and device ID match the current info table entry */
		if (info_table->id.mfr == id.mfr && info_table->id.dev == id.dev) {
			/* If matched, store the SPI NAND information */
			info.name = info_table->name;
			info.id = info_table->id;
			info.page_size = info_table->page_size;
			info.spare_size = info_table->spare_size;
			info.pages_per_block = info_table->pages_per_block;
			info.blocks_per_die = info_table->blocks_per_die;
			info.planes_per_die = info_table->planes_per_die;
			info.ndies = info_table->ndies;
			info.mode = info_table->mode;
			return 0; /* Return success */
		}
	}

	tx[0] = OPCODE_READ_ID;									  /* Command to read SPI NAND ID */
	r = sunxi_spi_transfer(spi, SPI_IO_SINGLE, tx, 2, rx, 5); /* Perform SPI transfer */
	if (r < 0)
		return r;

	printk_debug("rx: 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x\n", rx[0], rx[1], rx[2], rx[3], rx[4]);

	/* Check if the first byte of the received data is 0xff */
	if (rx[0] == 0xff) {
		rxp = rx + 1; /* If yes, shift the receive pointer by one byte (dummy data) */
	} else {
		rxp = rx;
	}

	id.mfr = rxp[0]; /* Set the manufacturer ID */
	for (i = 0; i < ARRAY_SIZE(spi_nand_infos); i++) {
		info_table = (spi_nand_info_t *) &spi_nand_infos[i]; /* Get a pointer to the current info table entry */
		if (info_table->id.dlen == 2) {
			id.dev = (((uint16_t) rxp[1]) << 8 | rxp[2]); /* Set the device ID (16-bit) */
		} else {
			id.dev = rxp[1]; /* Set the device ID (8-bit) */
		}
		/* Check if the manufacturer ID and device ID match the current info table entry */
		if (info_table->id.mfr == id.mfr && info_table->id.dev == id.dev) {
			/* If matched, store the SPI NAND information */
			info.name = info_table->name;
			info.id = info_table->id;
			info.page_size = info_table->page_size;
			info.spare_size = info_table->spare_size;
			info.pages_per_block = info_table->pages_per_block;
			info.blocks_per_die = info_table->blocks_per_die;
			info.planes_per_die = info_table->planes_per_die;
			info.ndies = info_table->ndies;
			info.mode = info_table->mode;
			return 0; /* Return success */
		}
	}

	printk_error("SPI-NAND: unknown mfr:0x%02x dev:0x%04x\n", id.mfr, id.dev);

	return -1; /* Return failure */
}

/**
 * Perform a reset operation on the SPI NAND.
 *
 * @param spi Pointer to the sunxi_spi_t structure.
 * @return 0 on success, -1 on failure.
 */
__attribute__((unused)) static int spi_nand_reset(sunxi_spi_t *spi) {
	uint8_t tx[1]; /* Transmit buffer */
	int r;		   /* Return value */

	tx[0] = OPCODE_RESET;									 /* Command to reset SPI NAND */
	r = sunxi_spi_transfer(spi, SPI_IO_SINGLE, tx, 1, 0, 0); /* Perform SPI transfer */
	if (r < 0)
		return -1;

	udelay(100 * 1000); /* Delay for 100 milliseconds */

	return 0; /* Return success */
}

/**
 * Get configuration value from SPI NAND at specified address.
 *
 * @param spi Pointer to the sunxi_spi_t structure.
 * @param addr Address to read from.
 * @param val Pointer to store the configuration value.
 * @return 0 on success, -1 on failure.
 */
static int spi_nand_get_config(sunxi_spi_t *spi, uint8_t addr, uint8_t *val) {
	uint8_t tx[2]; /* Transmit buffer */
	int r;		   /* Return value */

	tx[0] = OPCODE_READ_STATUS;								   /* Command to read status register */
	tx[1] = addr;											   /* Address to read from */
	r = sunxi_spi_transfer(spi, SPI_IO_SINGLE, tx, 2, val, 1); /* Perform SPI transfer */
	if (r < 0)
		return -1;

	return 0; /* Return success */
}

/**
 * Set configuration value to SPI NAND at specified address.
 *
 * @param spi Pointer to the sunxi_spi_t structure.
 * @param addr Address to write to.
 * @param val Value to be written.
 * @return 0 on success, -1 on failure.
 */
static int spi_nand_set_config(sunxi_spi_t *spi, uint8_t addr, uint8_t val) {
	uint8_t tx[3]; /* Transmit buffer */
	int r;		   /* Return value */

	tx[0] = OPCODE_WRITE_STATUS;							 /* Command to write status register */
	tx[1] = addr;											 /* Address to write to */
	tx[2] = val;											 /* Value to be written */
	r = sunxi_spi_transfer(spi, SPI_IO_SINGLE, tx, 3, 0, 0); /* Perform SPI transfer */
	if (r < 0)
		return -1;

	return 0; /* Return success */
}

/**
 * Wait until SPI NAND is not busy.
 *
 * @param spi Pointer to the sunxi_spi_t structure.
 */
static bool spi_nand_wait_while_busy(sunxi_spi_t *spi) {
	uint32_t timeout = 0xffff;
	uint8_t tx[2]; /* Transmit buffer */
	uint8_t rx[1]; /* Receive buffer */
	int r;		   /* Return value */

	tx[0] = OPCODE_READ_STATUS; /* Command to read status register */
	tx[1] = 0xc0;				/* SR3 address */
	rx[0] = 0x00;

	do {
		r = sunxi_spi_transfer(spi, SPI_IO_SINGLE, tx, 2, rx, 1); /* Perform SPI transfer */
		if (r < 0)
			break;
		timeout--;
		if (!timeout) {
			printk_warning("SPI NAND: wait busy timeout\n");
			return false;
		}
	} while ((rx[0] & 0x1) == 0x1); /* Check SR3 Busy bit */

	return true;
}

/**
 * Detect and initialize SPI NAND flash.
 *
 * @param spi Pointer to the sunxi_spi_t structure.
 * @return 0 on success, -1 on failure.
 */
int spi_nand_detect(sunxi_spi_t *spi) {
	uint8_t val; /* Configuration value */

	spi_nand_reset(spi);
	if (!spi_nand_wait_while_busy(spi))
		return -1;

	if (spi_nand_info(spi) == 0) {
		if ((spi_nand_get_config(spi, CONFIG_ADDR_PROTECT, &val) == 0) && (val != 0x0)) {
			spi_nand_set_config(spi, CONFIG_ADDR_PROTECT, 0x0);
			spi_nand_wait_while_busy(spi);
		}

		// Disable buffer mode on Winbond (enable continuous)
		if (info.id.mfr == (uint8_t) SPI_NAND_MFR_WINBOND) {
			if ((spi_nand_get_config(spi, CONFIG_ADDR_OTP, &val) == 0) && (val != 0x0)) {
				val &= ~CONFIG_POS_BUF;
				spi_nand_set_config(spi, CONFIG_ADDR_OTP, val);
				spi_nand_wait_while_busy(spi);
			}
		}

		if (info.id.mfr == (uint8_t) SPI_NAND_MFR_GIGADEVICE || info.id.mfr == (uint8_t) SPI_NAND_MFR_FORESEE || info.id.mfr == (uint8_t) SPI_NAND_MFR_XTX) {
			if ((spi_nand_get_config(spi, CONFIG_ADDR_OTP, &val) == 0) && !(val & 0x01)) {
				printk_debug("SPI-NAND: enable Quad mode\n");
				val |= (1 << 0);
				spi_nand_set_config(spi, CONFIG_ADDR_OTP, val);
				spi_nand_wait_while_busy(spi);
			}
		}

		printk_info("SPI-NAND: %s detected\n", info.name);

		return 0; /* Return success */
	}

	printk_error("SPI-NAND: flash not found\n");
	return -1; /* Return failure */
}

/**
 * Load a page from SPI NAND flash at the specified offset.
 *
 * @param spi Pointer to the sunxi_spi_t structure.
 * @param offset Offset of the page to load.
 * @return 0 on success, -1 on failure.
 */
static int spi_nand_load_page(sunxi_spi_t *spi, uint32_t offset) {
	uint32_t pa;   /* Page address */
	uint8_t tx[4]; /* Transmit buffer */

	pa = offset / info.page_size; /* Calculate page address */

	tx[0] = OPCODE_READ_PAGE;	  /* Command to read page */
	tx[1] = (uint8_t) (pa >> 16); /* High byte of page address */
	tx[2] = (uint8_t) (pa >> 8);  /* Middle byte of page address */
	tx[3] = (uint8_t) (pa >> 0);  /* Low byte of page address */

	sunxi_spi_transfer(spi, SPI_IO_SINGLE, tx, 4, 0, 0); /* Perform SPI transfer */
	spi_nand_wait_while_busy(spi);						 /* Wait until SPI NAND is not busy */

	return 0; /* Return success */
}

/**
 * Read data from SPI NAND flash.
 *
 * @param spi Pointer to the sunxi_spi_t structure.
 * @param buf Pointer to the buffer to store the read data.
 * @param addr Starting address to read from.
 * @param rxlen Number of bytes to read.
 * @return Number of bytes read on success, -1 on failure.
 */
uint32_t spi_nand_read(sunxi_spi_t *spi, uint8_t *buf, uint32_t addr, uint32_t rxlen) {
	uint32_t address = addr; /* Current address */
	uint32_t cnt = rxlen;	 /* Remaining bytes to read */
	uint32_t n;				 /* Number of bytes to read in each iteration */
	uint32_t len = 0;		 /* Total number of bytes read */
	uint32_t ca;			 /* Current address within a page */
	uint32_t txlen = 4;		 /* Transmit buffer length */
	uint8_t tx[6];			 /* Transmit buffer */

	int read_opcode = OPCODE_READ; /* Read opcode */
	switch (info.mode) {
		case SPI_IO_SINGLE:
			read_opcode = OPCODE_READ;
			break;
		case SPI_IO_DUAL_RX:
			read_opcode = OPCODE_FAST_READ_DUAL_O;
			break;
		case SPI_IO_QUAD_RX:
			read_opcode = OPCODE_FAST_READ_QUAD_O;
			break;
		case SPI_IO_QUAD_IO:
			read_opcode = OPCODE_FAST_READ_QUAD_IO;
			txlen = 5; /* Quad IO has 2 dummy bytes */
			break;
		default:
			printk_error("spi_nand: invalid mode\n");
			return -1;
	};

	if (addr % info.page_size) {
		printk_error("spi_nand: address is not page-aligned\n");
		return -1;
	}

	if (info.id.mfr == SPI_NAND_MFR_GIGADEVICE || info.id.mfr == SPI_NAND_MFR_FORESEE || info.id.mfr == SPI_NAND_MFR_XTX) {
		while (cnt > 0) {
			ca = address & (info.page_size - 1);
			n = cnt > (info.page_size - ca) ? (info.page_size - ca) : cnt;

			spi_nand_load_page(spi, address);

			tx[0] = read_opcode;
			tx[1] = (uint8_t) (ca >> 8);
			tx[2] = (uint8_t) (ca >> 0);
			tx[3] = 0x0;

			sunxi_spi_transfer(spi, info.mode, tx, 4, buf, n);

			address += n;
			buf += n;
			len += n;
			cnt -= n;
		}
	} else {
		spi_nand_load_page(spi, addr);

		// With Winbond, we use continuous mode which has 1 more dummy
		// This allows us to not load each page
		if (info.id.mfr == SPI_NAND_MFR_WINBOND) {
			txlen++;
		}

		ca = address & (info.page_size - 1);

		tx[0] = read_opcode;
		tx[1] = (uint8_t) (ca >> 8);
		tx[2] = (uint8_t) (ca >> 0);
		tx[3] = 0x0;
		tx[4] = 0x0;

		sunxi_spi_transfer(spi, info.mode, tx, txlen, buf, rxlen);
	}

	return len; /* Return total number of bytes read */
}
