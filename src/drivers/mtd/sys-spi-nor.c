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

#include <sys-spi-nor.h>
#include <sys-spi.h>

static spi_nor_info_t info;

static const spi_nor_info_t spi_nor_info_table[] = {
		{"W25X40", 0xef3013, 512 * 1024, 4096, 1, 256, 3, NOR_OPCODE_READ, NOR_OPCODE_PROG, NOR_OPCODE_WREN, NOR_OPCODE_E4K, 0, NOR_OPCODE_E64K, 0},
		{"W25Q128JVEIQ", 0xefc018, 16 * 1024 * 1024, 4096, 1, 256, 3, NOR_OPCODE_READ, NOR_OPCODE_PROG, NOR_OPCODE_WREN, NOR_OPCODE_E4K, NOR_OPCODE_E32K, NOR_OPCODE_E64K, 0},
		{"GD25D10B", 0xc84011, 128 * 1024, 4096, 1, 256, 3, NOR_OPCODE_READ, NOR_OPCODE_PROG, NOR_OPCODE_WREN, NOR_OPCODE_E4K, NOR_OPCODE_E32K, NOR_OPCODE_E64K, 0},
};

/**
 * @brief Dump the contents of the SFDP (Serial Flash Discoverable Parameters) data structure.
 * 
 * This function prints the SFDP header, parameter headers, and the basic table information to the log.
 * It will provide detailed output about the SFDP format, including version information, the number of parameter headers,
 * and the contents of the basic table in the SFDP structure.
 * 
 * @param sfdp Pointer to an `sfdp_t` structure containing the SFDP data to be dumped.
 * 
 * @note This function is marked as `__attribute__((unused))` to avoid unused function warnings if not used.
 * 
 * @warning If the provided `sfdp` pointer is NULL, the function will print a trace log indicating the issue.
 */
__attribute__((unused)) static inline void spi_nor_dump_sfdp(const sfdp_t *sfdp) {
	if (sfdp == NULL) {
		printk_trace("SFDP data is NULL.\n");
		return;
	}

	printk_trace("SFDP Header:\n");
	printk_trace("  Signature: %c%c%c%c\n", sfdp->header.sign[0], sfdp->header.sign[1], sfdp->header.sign[2], sfdp->header.sign[3]);
	printk_trace("  Minor version: %u\n", sfdp->header.minor);
	printk_trace("  Major version: %u\n", sfdp->header.major);
	printk_trace("  Number of Parameter Headers: %u\n", sfdp->header.nph);
	printk_trace("  Unused: 0x%02X\n", sfdp->header.unused);

	printk_trace("SFDP Parameter Headers:\n");
	for (int i = 0; i < sfdp->header.nph; i++) {
		printk_trace("  Parameter Header #%d:\n", i + 1);
		printk_trace("    IDLSB: 0x%02X\n", sfdp->parameter_header[i].idlsb);
		printk_trace("    Minor version: %u\n", sfdp->parameter_header[i].minor);
		printk_trace("    Major version: %u\n", sfdp->parameter_header[i].major);
		printk_trace("    Length: %u\n", sfdp->parameter_header[i].length);
		printk_trace("    PTP: 0x%02X 0x%02X 0x%02X\n", sfdp->parameter_header[i].ptp[0], sfdp->parameter_header[i].ptp[1], sfdp->parameter_header[i].ptp[2]);
		printk_trace("    IDMSB: 0x%02X\n", sfdp->parameter_header[i].idmsb);
	}

	printk_trace("SFDP Basic Table:\n");
	printk_trace("  Minor version: %u\n", sfdp->basic_table.minor);
	printk_trace("  Major version: %u\n", sfdp->basic_table.major);
	printk_trace("  Table (16 x 4 bytes):\n");
	for (int i = 0; i < 16; i++) {
		printk_trace("    ");
		for (int j = 0; j < 4; j++) { printk(LOG_LEVEL_MUTE, "0x%02X ", sfdp->basic_table.table[i * 4 + j]); }
		printk(LOG_LEVEL_MUTE, "\n");
	}
}

/**
 * @brief Read the SFDP (Serial Flash Discoverable Parameters) data from a SPI NOR Flash chip.
 * 
 * This function sends the appropriate commands to the SPI NOR Flash to retrieve the SFDP information.
 * The SFDP is a standardized way for flash devices to expose their capabilities and characteristics.
 * It reads the SFDP header and parameter headers, then fetches the basic table if the parameter header conditions are met.
 * 
 * @param spi Pointer to a `sunxi_spi_t` structure representing the SPI device.
 * @param sfdp Pointer to a `sfdp_t` structure where the SFDP data will be stored.
 * 
 * @return 1 if the SFDP data was successfully read, 0 if there was an error or the data was invalid.
 */
static inline int spi_nor_read_sfdp(sunxi_spi_t *spi, sfdp_t *sfdp) {
	uint32_t addr;
	uint8_t tx[5];
	int i;

	memset(sfdp, 0, sizeof(sfdp_t));
	tx[0] = NOR_OPCODE_SFDP;
	tx[1] = 0x0;
	tx[2] = 0x0;
	tx[3] = 0x0;
	tx[4] = 0x0;
	if (!sunxi_spi_transfer(spi, SPI_IO_SINGLE, tx, 5, &sfdp->header, sizeof(sfdp_header_t)))
		return 0;

	if ((sfdp->header.sign[0] != 'S') || (sfdp->header.sign[1] != 'F') || (sfdp->header.sign[2] != 'D') || (sfdp->header.sign[3] != 'P'))
		return 0;

	sfdp->header.nph = sfdp->header.nph > SFDP_MAX_NPH ? sfdp->header.nph + 1 : SFDP_MAX_NPH;
	for (i = 0; i < sfdp->header.nph; i++) {
		addr = i * sizeof(sfdp_parameter_header_t) + sizeof(sfdp_header_t);
		tx[0] = NOR_OPCODE_SFDP;
		tx[1] = (addr >> 16) & 0xff;
		tx[2] = (addr >> 8) & 0xff;
		tx[3] = (addr >> 0) & 0xff;
		tx[4] = 0x0;
		if (!sunxi_spi_transfer(spi, SPI_IO_SINGLE, tx, 5, &sfdp->parameter_header[i], sizeof(sfdp_parameter_header_t)))
			return 0;
	}
	for (i = 0; i < sfdp->header.nph; i++) {
		if ((sfdp->parameter_header[i].idlsb == 0x00) && (sfdp->parameter_header[i].idmsb == 0xff)) {
			addr = (sfdp->parameter_header[i].ptp[0] << 0) | (sfdp->parameter_header[i].ptp[1] << 8) | (sfdp->parameter_header[i].ptp[2] << 16);
			tx[0] = NOR_OPCODE_SFDP;
			tx[1] = (addr >> 16) & 0xff;
			tx[2] = (addr >> 8) & 0xff;
			tx[3] = (addr >> 0) & 0xff;
			tx[4] = 0x0;
			if (sunxi_spi_transfer(spi, SPI_IO_SINGLE, tx, 5, &sfdp->basic_table.table[0], sfdp->parameter_header[i].length * 4)) {
				sfdp->basic_table.major = sfdp->parameter_header[i].major;
				sfdp->basic_table.minor = sfdp->parameter_header[i].minor;
				return 1;
			}
		}
	}
	return 0;
}

/**
 * @brief Read the identification (ID) of the SPI NOR Flash chip.
 * 
 * This function sends the "Read ID" command (RDID) to the SPI NOR Flash chip and reads back its 3-byte identification.
 * The identification is stored in the provided `id` variable as a 24-bit value, combining the 3 received bytes.
 * 
 * @param spi Pointer to a `sunxi_spi_t` structure representing the SPI device.
 * @param id Pointer to a `uint32_t` variable where the 24-bit chip ID will be stored.
 * 
 * @return 1 if the ID was successfully read, 0 if the transfer failed.
 */
static inline int spinor_read_id(sunxi_spi_t *spi, uint32_t *id) {
	uint8_t tx[1];
	uint8_t rx[3];

	tx[0] = NOR_OPCODE_RDID;
	if (!sunxi_spi_transfer(spi, SPI_IO_SINGLE, tx, 1, rx, 3))
		return 0;
	*id = (rx[0] << 16) | (rx[1] << 8) | (rx[2] << 0);
	return 1;
}

/**
 * @brief Read the status register of the SPI NOR Flash chip.
 * 
 * This function sends the "Read Status Register" command (RDSR) to the SPI NOR Flash chip and reads back a single byte
 * representing the current status register value.
 * 
 * @param spi Pointer to a `sunxi_spi_t` structure representing the SPI device.
 * 
 * @return The 1-byte status register value returned by the NOR Flash chip.
 */
static inline uint8_t spi_nor_read_status_register(sunxi_spi_t *spi) {
	uint8_t tx = NOR_OPCODE_RDSR;
	uint8_t rx = 0;

	sunxi_spi_transfer(spi, SPI_IO_SINGLE, &tx, 1, &rx, 1);
	return rx;
}

/**
 * @brief Write to the status register of the SPI NOR Flash chip.
 * 
 * This function sends the "Write Status Register" command (WRSR) to the SPI NOR Flash chip, followed by a byte
 * containing the new status register value to be written.
 * 
 * @param spi Pointer to a `sunxi_spi_t` structure representing the SPI device.
 * @param sr The new status register value to write to the NOR Flash chip.
 */
static inline void spi_nor_write_status_register(sunxi_spi_t *spi, uint8_t sr) {
	uint8_t tx[2];

	tx[0] = NOR_OPCODE_WRSR;
	tx[1] = sr;

	sunxi_spi_transfer(spi, SPI_IO_SINGLE, &tx, 2, NULL, 0);
}

/**
 * @brief Wait for SPI NOR Flash to finish operation by checking its "busy" status.
 * 
 * This function continuously checks the NOR Flash status register until the "busy" bit is cleared (i.e., operation is complete).
 * It reads the status register to determine whether the NOR Flash is still in a busy state.
 * 
 * @param spi Pointer to a `sunxi_spi_t` structure representing the SPI device.
 */
static inline void spi_nor_wait_for_busy(sunxi_spi_t *spi) {
	uint32_t timeout = 0xffff;
	while (((spi_nor_read_status_register(spi) & 0x1) == 0x1)) {
		timeout--;
		if (!timeout) {
			printk_warning("SPI NAND: wait busy timeout\n");
			return;
		}
	}
}

/**
 * @brief Reset the SPI NOR Flash chip.
 * 
 * This function sends a specific command sequence (0x66 and 0x99) to reset the NOR Flash chip.
 * These two bytes are typically used to return the NOR Flash to its initial state. 
 * It is often used during chip initialization or recovery processes.
 * 
 * @param spi Pointer to a `sunxi_spi_t` structure representing the SPI device.
 */
static inline void spi_nor_chip_reset(sunxi_spi_t *spi) {
	uint8_t tx[2];

	tx[0] = 0x66;
	tx[1] = 0x99;

	sunxi_spi_transfer(spi, SPI_IO_SINGLE, &tx, 2, NULL, 0);
}

/**
 * @brief Enable write operations on the SPI NOR Flash chip.
 * 
 * This function sends the "Write Enable" command (usually 0x06) to enable write operations on the NOR Flash chip.
 * The Write Enable command must be issued before any write operation can take place.
 * 
 * @param spi Pointer to a `sunxi_spi_t` structure representing the SPI device.
 */
static inline void spi_nor_set_write_enable(sunxi_spi_t *spi) {
	sunxi_spi_transfer(spi, SPI_IO_SINGLE, (void *) info.opcode_write_enable, 1, NULL, 0);
}


/**
 * @brief Retrieves the information of the SPI NOR flash.
 * 
 * This function reads the identification (ID) of the SPI NOR flash and attempts
 * to fetch its detailed parameters using the Serial Flash Discoverable Parameters (SFDP)
 
 * table. Based on the flash type and capacity, it configures the address length, 
 * erase opcodes, write granularity, and other flash-specific parameters. If the SFDP
 * table is unavailable, the function will fall back on a predefined lookup table using 
 * the flash's ID to find a match. It then populates the `info` structure with the gathered 
 * information.
 * 
 * @param spi The SPI interface structure representing the SPI controller to interact with the NOR flash.
 * 
 * @return 
 *  - 1 if the flash information was successfully retrieved.
 *  - 0 if the flash ID is not recognized or the SPI NOR flash could not be detected.
 * 
 * @note If the SPI NOR flash is recognized using its ID, this function will populate the `info`
 *       structure with its capabilities such as capacity, address length, erase block size,
 *       and supported opcodes. If the SFDP is valid, it also provides more granular details
 *       such as the opcode for 4K, 32K, 64K, and 256K erases, as well as write granularity.
 * 
 * @see spinor_read_id(), spi_nor_read_sfdp(), spi_nor_dump_sfdp(), NOR_OPCODE_WREN, NOR_OPCODE_READ, NOR_OPCODE_PROG
 */
static inline int spi_nor_get_info(sunxi_spi_t *spi) {
	sfdp_t sfdp;
	spi_nor_info_t *tmp_info;
	uint32_t v, i, id = 0x0;

	spinor_read_id(spi, &id);
	info.id = id;

	if (spi_nor_read_sfdp(spi, &sfdp)) {
		info.name = "SPDF";
#if LOG_LEVEL_DEFAULT >= LOG_LEVEL_TRACE
		spi_nor_dump_sfdp(&sfdp);
#endif

		v = (sfdp.basic_table.table[7] << 24) | (sfdp.basic_table.table[6] << 16) | (sfdp.basic_table.table[5] << 8) | (sfdp.basic_table.table[4] << 0);
		if (v & (1 << 31)) {
			v &= 0x7fffffff;
			info.capacity = 1 << (v - 3);
		} else {
			info.capacity = (v + 1) >> 3;
		}
		/* Basic flash parameter table 1th dword */
		v = (sfdp.basic_table.table[3] << 24) | (sfdp.basic_table.table[2] << 16) | (sfdp.basic_table.table[1] << 8) | (sfdp.basic_table.table[0] << 0);

		if ((info.capacity <= (16 * 1024 * 1024)) && (((v >> 17) & 0x3) != 0x2))
			info.address_length = 3;
		else
			info.address_length = 4;
		if (((v >> 0) & 0x3) == 0x1)
			info.opcode_erase_4k = (v >> 8) & 0xff;
		else
			info.opcode_erase_4k = 0x00;
		info.opcode_erase_32k = 0x00;
		info.opcode_erase_64k = 0x00;
		info.opcode_erase_256k = 0x00;

		/* Basic flash parameter table 8th dword */
		v = (sfdp.basic_table.table[31] << 24) | (sfdp.basic_table.table[30] << 16) | (sfdp.basic_table.table[29] << 8) | (sfdp.basic_table.table[28] << 0);

		switch ((v >> 0) & 0xff) {
			case 12:
				info.opcode_erase_4k = (v >> 8) & 0xff;
				break;
			case 15:
				info.opcode_erase_32k = (v >> 8) & 0xff;
				break;
			case 16:
				info.opcode_erase_64k = (v >> 8) & 0xff;
				break;
			case 18:
				info.opcode_erase_256k = (v >> 8) & 0xff;
				break;
			default:
				break;
		}
		switch ((v >> 16) & 0xff) {
			case 12:
				info.opcode_erase_4k = (v >> 24) & 0xff;
				break;
			case 15:
				info.opcode_erase_32k = (v >> 24) & 0xff;
				break;
			case 16:
				info.opcode_erase_64k = (v >> 24) & 0xff;
				break;
			case 18:
				info.opcode_erase_256k = (v >> 24) & 0xff;
				break;
			default:
				break;
		}

		/* Basic flash parameter table 9th dword */
		v = (sfdp.basic_table.table[35] << 24) | (sfdp.basic_table.table[34] << 16) | (sfdp.basic_table.table[33] << 8) | (sfdp.basic_table.table[32] << 0);
		switch ((v >> 0) & 0xff) {
			case 12:
				info.opcode_erase_4k = (v >> 8) & 0xff;
				break;
			case 15:
				info.opcode_erase_32k = (v >> 8) & 0xff;
				break;
			case 16:
				info.opcode_erase_64k = (v >> 8) & 0xff;
				break;
			case 18:
				info.opcode_erase_256k = (v >> 8) & 0xff;
				break;
			default:
				break;
		}
		switch ((v >> 16) & 0xff) {
			case 12:
				info.opcode_erase_4k = (v >> 24) & 0xff;
				break;
			case 15:
				info.opcode_erase_32k = (v >> 24) & 0xff;
				break;
			case 16:
				info.opcode_erase_64k = (v >> 24) & 0xff;
				break;
			case 18:
				info.opcode_erase_256k = (v >> 24) & 0xff;
				break;
			default:
				break;
		}
		if (info.opcode_erase_4k != 0x00)
			info.blksz = 4096;
		else if (info.opcode_erase_32k != 0x00)
			info.blksz = 32768;
		else if (info.opcode_erase_64k != 0x00)
			info.blksz = 65536;
		else if (info.opcode_erase_256k != 0x00)
			info.blksz = 262144;

		info.opcode_write_enable = NOR_OPCODE_WREN;
		info.read_granularity = 1;
		info.opcode_read = NOR_OPCODE_READ;

		if ((sfdp.basic_table.major == 1) && (sfdp.basic_table.minor < 5)) {
			/* Basic flash parameter table 1th dword */
			v = (sfdp.basic_table.table[3] << 24) | (sfdp.basic_table.table[2] << 16) | (sfdp.basic_table.table[1] << 8) | (sfdp.basic_table.table[0] << 0);
			if ((v >> 2) & 0x1)
				info.write_granularity = 64;
			else
				info.write_granularity = 1;
		} else if ((sfdp.basic_table.major == 1) && (sfdp.basic_table.minor >= 5)) {
			/* Basic flash parameter table 11th dword */
			v = (sfdp.basic_table.table[43] << 24) | (sfdp.basic_table.table[42] << 16) | (sfdp.basic_table.table[41] << 8) | (sfdp.basic_table.table[40] << 0);
			info.write_granularity = 1 << ((v >> 4) & 0xf);
		}
		info.opcode_write = NOR_OPCODE_PROG;
		return 1;
	} else if ((id != 0xffffff) && (id != 0)) {
		for (i = 0; i < ARRAY_SIZE(spi_nor_info_table); i++) {
			tmp_info = &spi_nor_info_table[i];
			if (id == tmp_info->id) {
				memcpy(&info, tmp_info, sizeof(spi_nor_info_t));
				return 1;
			}
		}
		printk_error("The spi nor flash '0x%x' is not yet supported\r\n", id);
	}
	return 0;
}

/**
 * @brief Reads a specified number of bytes from the SPI NOR flash memory.
 *
 * This function sends the read command and address to the SPI NOR flash
 * memory and retrieves the requested data. It supports different address
 * lengths (3 or 4 bytes) based on the configuration in the `info` structure.
 *
 * @param[in] spi Pointer to the SPI interface structure.
 * @param[in] addr The starting address to read data from in the SPI NOR.
 * @param[out] buf Pointer to the buffer where the read data will be stored.
 * @param[in] count The number of bytes to read from the SPI NOR.
 *
 * @return None
 * 
 * @note This function uses the `sunxi_spi_transfer` function to perform
 *       the SPI data transfer. The number of bytes transferred depends 
 *       on the address length configuration, which is either 3 or 4 bytes.
 * 
 * @details 
 * This function first checks the `address_length` configuration (from 
 * `info`) to determine if the address is 3 bytes or 4 bytes long. Based 
 * on this configuration, it sends the appropriate number of address bytes
 * and the read opcode to the SPI NOR. The data is then transferred to 
 * the provided buffer. The function supports 3-byte or 4-byte address
 * modes, but any other address length is not supported.
 */
static void spi_nor_read_bytes(sunxi_spi_t *spi, uint32_t addr, uint8_t *buf, uint32_t count) {
	uint8_t tx[5];
	switch (info.address_length) {
		case 3:
			tx[0] = info.opcode_read;
			tx[1] = (uint8_t) (addr >> 16);
			tx[2] = (uint8_t) (addr >> 8);
			tx[3] = (uint8_t) (addr >> 0);
			sunxi_spi_transfer(spi, SPI_IO_SINGLE, tx, 4, buf, count);
			break;
		case 4:
			tx[0] = info.opcode_read;
			tx[1] = (uint8_t) (addr >> 24);
			tx[2] = (uint8_t) (addr >> 16);
			tx[3] = (uint8_t) (addr >> 8);
			tx[4] = (uint8_t) (addr >> 0);
			sunxi_spi_transfer(spi, SPI_IO_SINGLE, tx, 5, buf, count);
			break;
		default:
			break;
	}
}

/**
 * @brief Detects the presence of an SPI NOR flash chip.
 *
 * This function attempts to identify and initialize the SPI NOR flash chip
 * connected to the specified SPI interface. It resets the chip, waits for it
 * to be ready, and retrieves its information. If successful, it logs the chip ID 
 * and its capacity.
 *
 * @param[in] spi Pointer to the SPI interface structure. This should be 
 *                initialized and configured before calling this function.
 *
 * @return 
 *         - 0 on successful detection and initialization of the SPI NOR chip.
 *         - -1 if no supported SPI NOR chip is found.
 *
 * @details The function performs the following steps:
 *          1. Resets the SPI NOR chip.
 *          2. Waits until the chip is not busy.
 *          3. Checks the chip information. If no supported chip is found, 
 *             a warning is logged and the function returns -1.
 *          4. If a chip is detected, its ID and capacity are logged to 
 *             inform the user.
 */
int spi_nor_detect(sunxi_spi_t *spi) {
	spi_nor_chip_reset(spi);
	spi_nor_wait_for_busy(spi);

	if (!spi_nor_get_info(spi)) {
		printk_warning("SPI NOR: Can not find any supported SPI NOR\n");
		return -1;
	}

	printk_info("SPI NOR: detect spi nor id=0x%06x capacity=%dMB\n", info.id, info.capacity / 1024 / 1024);

	return 0;
}

/**
 * @brief Reads a block or multiple blocks of data from the SPI NAND flash memory.
 *
 * This function reads one or more contiguous blocks from the SPI NAND flash memory
 * into a provided buffer. It handles reading the data in chunks defined by the 
 * read granularity and performs the necessary address calculations to handle 
 * multiple blocks.
 *
 * @param[in] spi Pointer to the SPI interface structure.
 * @param[out] buf Pointer to the buffer where the read data will be stored.
 * @param[in] blk_no The starting block number from which to read data.
 * @param[in] blk_cnt The number of blocks to read from the SPI NAND.
 *
 * @return The number of blocks successfully read. In case of an error, it will 
 *         return the requested block count, indicating the operation was completed.
 *
 * @note This function assumes that the buffer provided by the caller is large 
 *       enough to accommodate the data being read. If the block count is large,
 *       ensure the buffer has sufficient space for all the blocks.
 *
 * @details 
 * The function reads data from the SPI NAND flash memory in chunks based on the 
 * configured read granularity (`info.read_granularity`). It waits for the SPI
 * bus to be ready before each read operation. The function will continue reading 
 * until the requested number of blocks has been fetched. If the data to be read 
 * exceeds the maximum read length (0x7FFFFFFF bytes), it adjusts the size of 
 * each read operation accordingly.
 */
uint32_t spi_nor_read_block(sunxi_spi_t *spi, uint8_t *buf, uint32_t blk_no, uint32_t blk_cnt) {
	uint32_t addr = blk_no * info.blksz;
	uint32_t cnt = blk_cnt * info.blksz;

	uint8_t *pbuf = buf;
	uint32_t len;

	if (info.read_granularity == 1)
		len = (cnt < 0x7fffffff) ? cnt : 0x7fffffff;
	else
		len = info.read_granularity;
	while (cnt > 0) {
		spi_nor_wait_for_busy(spi);
		spi_nor_read_bytes(spi, addr, pbuf, len);
		addr += len;
		pbuf += len;
		cnt -= len;
	}
	return blk_cnt;
}

/**
 * @brief Reads data from the SPI NOR flash memory.
 *
 * This function reads a specified length of data from a given address in the 
 * SPI NOR flash memory into a provided buffer. The reading is performed in
 * blocks, and it handles cases where the read address is not aligned to
 * the block size.
 *
 * @param[in] spi Pointer to the SPI interface structure.
 * @param[out] buf Pointer to the buffer where the read data will be stored.
 * @param[in] addr The starting address from which to read data in the SPI NOR.
 * @param[in] rxlen The number of bytes to read from the SPI NOR.
 *
 * @return The number of bytes successfully read from the SPI NOR. This can 
 *         be less than `rxlen` if an error occurs or if the end of the 
 *         memory space is reached.
 *
 * @note This function assumes that the buffer provided by the caller is 
 *       large enough to accommodate the data being read.
 * 
 * @details The function first checks if the read address is misaligned with
 *          the block size. If so, it reads a partial block. Then, it reads
 *          as many complete blocks as possible before potentially reading 
 *          another partial block at the end.
 */
uint32_t spi_nor_read(sunxi_spi_t *spi, uint8_t *buf, uint32_t addr, uint32_t rxlen) {
	u64_t blksz = info.blksz;
	u64_t blkno, len, tmp;
	u64_t ret = 0;

	blkno = addr / blksz;
	tmp = addr % blksz;

	if (tmp > 0) {
		len = blksz - tmp;
		if (rxlen < len)
			len = rxlen;
		if (spi_nor_read_block(spi, &buf[0], blkno, 1) != 1)
			return ret;
		memcpy((void *) buf, (const void *) (&buf[tmp]), len);
		buf += len;
		rxlen -= len;
		ret += len;
		blkno += 1;
	}

	tmp = rxlen / blksz;

	if (tmp > 0) {
		len = tmp * blksz;
		if (spi_nor_read_block(spi, buf, blkno, tmp) != tmp)
			return ret;
		buf += len;
		rxlen -= len;
		ret += len;
		blkno += tmp;
	}

	if (rxlen > 0) {
		len = rxlen;
		if (spi_nor_read_block(spi, &buf[0], blkno, 1) != 1)
			return ret;
		memcpy((void *) buf, (const void *) (&buf[0]), len);
		ret += len;
	}
	return ret;
}
