/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __SYS_SPI_NOR_H__
#define __SYS_SPI_NOR_H__

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <sys-clk.h>
#include <sys-gpio.h>
#include <sys-spi.h>

#include <log.h>

#ifdef __cplusplus
extern "C" {
#endif// __cplusplus

#define SFDP_MAX_NPH (6)

/**
 * @struct sfdp_header_t
 * @brief SFDP Header structure.
 * 
 * This structure represents the header of the Serial Flash Discoverable Parameters (SFDP) data.
 * It contains the signature, version information, and the number of parameter headers.
 */
typedef struct sfdp_header {
	uint8_t sign[4]; /**< Signature for the SFDP header, typically "SFDP". */
	uint8_t minor;	 /**< Minor version number of the SFDP specification. */
	uint8_t major;	 /**< Major version number of the SFDP specification. */
	uint8_t nph;	 /**< Number of Parameter Headers present in the SFDP data. */
	uint8_t unused;	 /**< Reserved or unused byte(s) for future extensions or alignment. */
} sfdp_header_t;

/**
 * @struct sfdp_parameter_header_t
 * @brief SFDP Parameter Header structure.
 * 
 * This structure defines the header for each parameter table in the SFDP data.
 * It includes the parameter ID (split into MSB and LSB), versioning information, 
 * and a pointer to the parameter table.
 */
typedef struct sfdp_parameter_header {
	uint8_t idlsb;	/**< Least significant byte of the parameter ID. */
	uint8_t minor;	/**< Minor version number of the parameter. */
	uint8_t major;	/**< Major version number of the parameter. */
	uint8_t length; /**< Length of the parameter table in bytes. */
	uint8_t ptp[3]; /**< 3-byte pointer to the parameter table. */
	uint8_t idmsb;	/**< Most significant byte of the parameter ID. */
} sfdp_parameter_header_t;

/**
 * @struct sfdp_basic_table_t
 * @brief SFDP Basic Parameter Table structure.
 * 
 * This structure represents the basic parameter table defined in the SFDP data. 
 * It contains the table version and a list of basic parameters.
 */
typedef struct sfdp_basic_table {
	uint8_t minor;		   /**< Minor version of the basic parameter table. */
	uint8_t major;		   /**< Major version of the basic parameter table. */
	uint8_t table[16 * 4]; /**< Basic parameter table (16 entries, each 4 bytes). */
} sfdp_basic_table_t;

/**
 * @struct sfdp_t
 * @brief Full SFDP structure combining header, parameter headers, and basic table.
 * 
 * This structure combines the SFDP header, an array of parameter headers, 
 * and the basic parameter table into a single structure for comprehensive SFDP data.
 */
typedef struct sfdp {
	sfdp_header_t header;									/**< SFDP header containing signature and version info. */
	sfdp_parameter_header_t parameter_header[SFDP_MAX_NPH]; /**< Array of parameter headers. */
	sfdp_basic_table_t basic_table;							/**< Basic parameter table containing the flash parameters. */
} sfdp_t;

/**
 * @struct spi_nor_info_t
 * @brief Structure containing information about a specific SPI NOR Flash device.
 * 
 * This structure holds essential information about the configuration, 
 * operational capabilities, and specific commands for a SPI NOR Flash memory device.
 */
typedef struct spi_nor_info {
	char *name;					 /**< Name of the SPI NOR Flash device (e.g., "MX25L12835E"). */
	uint32_t id;				 /**< Unique device ID for the SPI NOR Flash (manufacturer and model). */
	uint32_t capacity;			 /**< Total capacity of the SPI NOR Flash (in bytes). */
	uint32_t blksz;				 /**< Block size of the SPI NOR Flash (in bytes). */
	uint32_t read_granularity;	 /**< Read granularity, smallest read unit (in bytes). */
	uint32_t write_granularity;	 /**< Write granularity, smallest write unit (in bytes). */
	uint8_t address_length;		 /**< Length of the address field in the command (in bytes). */
	uint8_t opcode_read;		 /**< Opcode for the read operation. */
	uint8_t opcode_write;		 /**< Opcode for the write operation. */
	uint8_t opcode_write_enable; /**< Opcode to enable write operations on the SPI NOR Flash. */
	uint8_t opcode_erase_4k;	 /**< Opcode to erase a 4K block of the SPI NOR Flash. */
	uint8_t opcode_erase_32k;	 /**< Opcode to erase a 32K block of the SPI NOR Flash. */
	uint8_t opcode_erase_64k;	 /**< Opcode to erase a 64K block of the SPI NOR Flash. */
	uint8_t opcode_erase_256k;	 /**< Opcode to erase a 256K block of the SPI NOR Flash. */
} spi_nor_info_t;

/**
 * @enum SPI_NOR_OPS
 * @brief Enumeration of SPI NOR Flash operation opcodes.
 * 
 * This enumeration defines the opcodes used for various operations on 
 * SPI NOR Flash memory. Each opcode corresponds to a specific command 
 * that can be issued to the NOR Flash for performing read, write, 
 * erase, and other operational tasks.
 */
enum SPI_NOR_OPS {
	NOR_OPCODE_SFDP = 0x5a,		/**< SFDP Read Command: Read Serial Flash Discoverable Parameters */
	NOR_OPCODE_RDID = 0x9f,		/**< Read ID Command: Retrieve the identity of the memory device */
	NOR_OPCODE_WRSR = 0x01,		/**< Write Status Register Command: Write to the status register */
	NOR_OPCODE_RDSR = 0x05,		/**< Read Status Register Command: Read the current status register */
	NOR_OPCODE_WREN = 0x06,		/**< Write Enable Command: Enable write operations on the memory */
	NOR_OPCODE_READ = 0x03,		/**< Read Data Command: Read data from the memory */
	NOR_OPCODE_PROG = 0x02,		/**< Page Program Command: Program data into a memory page */
	NOR_OPCODE_E4K = 0x20,		/**< 4K Block Erase Command: Erase a 4K block of memory */
	NOR_OPCODE_E32K = 0x52,		/**< 32K Block Erase Command: Erase a 32K block of memory */
	NOR_OPCODE_E64K = 0xd8,		/**< 64K Block Erase Command: Erase a 64K block of memory */
	NOR_OPCODE_ENTER_4B = 0xb7, /**< Enter 4-Byte Address Mode Command: Switch to 4-byte addressing mode */
	NOR_OPCODE_EXIT_4B = 0xe9,	/**< Exit 4-Byte Address Mode Command: Return to 3-byte addressing mode */
};

/**
 * @enum SPI_CMD_OPS
 * @brief Enumeration of SPI command operations.
 * 
 * This enumeration defines a set of commands used to control
 * the behavior of SPI operations. These commands may include 
 * initialization, selection, deselection, and data transmission 
 * commands to facilitate communication with peripheral devices.
 */
enum SPI_CMD_OPS {
	SPI_CMD_END = 0x00,			 /**< End Command: Mark the end of a command sequence */
	SPI_CMD_INIT = 0x01,		 /**< Initialization Command: Initialize the SPI device */
	SPI_CMD_SELECT = 0x02,		 /**< Select Command: Select the SPI device for communication */
	SPI_CMD_DESELECT = 0x03,	 /**< Deselect Command: Deselect the SPI device to end communication */
	SPI_CMD_FAST = 0x04,		 /**< Fast Command: Execute a fast operation or mode */
	SPI_CMD_TXBUF = 0x05,		 /**< Transmit Buffer Command: Transmit data from a buffer to the SPI device */
	SPI_CMD_RXBUF = 0x06,		 /**< Receive Buffer Command: Receive data from the SPI device to a buffer */
	SPI_CMD_SPINOR_WAIT = 0x07,	 /**< Wait Command for SPI NOR: Wait for the SPI NOR flash to complete its operation */
	SPI_CMD_SPINAND_WAIT = 0x08, /**< Wait Command for SPI NAND: Wait for the SPI NAND flash to complete its operation */
};

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
int spi_nor_detect(sunxi_spi_t *spi);

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
uint32_t spi_nor_read_block(sunxi_spi_t *spi, uint8_t *buf, uint32_t blk_no, uint32_t blk_cnt);

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
uint32_t spi_nor_read(sunxi_spi_t *spi, uint8_t *buf, uint32_t addr, uint32_t rxlen);

#ifdef __cplusplus
}
#endif// __cplusplus

#endif// __SYS_SPI_NOR_H__