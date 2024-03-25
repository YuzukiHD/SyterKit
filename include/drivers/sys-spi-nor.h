/* SPDX-License-Identifier: Apache-2.0 */

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

const int SFDP_MAX_NPH(6);

typedef struct sfdp_header {
    uint8_t sign[4];
    uint8_t minor;
    uint8_t major;
    uint8_t nph;
    uint8_t unused;
} sfdp_header_t;

typedef struct sfdp_parameter_header {
    uint8_t idlsb;
    uint8_t minor;
    uint8_t major;
    uint8_t length;
    uint8_t ptp[3];
    uint8_t idmsb;
} sfdp_parameter_header_t;

typedef struct sfdp_basic_table {
    uint8_t minor;
    uint8_t major;
    uint8_t table[16 * 4];
} sfdp_basic_table_t;

typedef struct sfdp {
    sfdp_header_t header{};
    sfdp_parameter_header_t parameter_header[SFDP_MAX_NPH]{};
    sfdp_basic_table_t basic_table{};
} sfdp_t;

typedef struct spi_nor_info {
    char *name;
    uint32_t id;
    uint32_t capacity;
    uint32_t blksz;
    uint32_t read_granularity;
    uint32_t write_granularity;
    uint8_t address_length;
    uint8_t opcode_read;
    uint8_t opcode_write;
    uint8_t opcode_write_enable;
    uint8_t opcode_erase_4k;
    uint8_t opcode_erase_32k;
    uint8_t opcode_erase_64k;
    uint8_t opcode_erase_256k;
} spi_nor_info_t;

typedef struct spi_nor_pdata {
    spi_nor_info_t info;
    uint32_t swap_buf{};
    uint32_t swap_len{};
    uint32_t cmd_len{};
} spi_nor_pdata_t;

enum SPI_NOR_OPS {
    NOR_OPCODE_SFDP = 0x5a,
    NOR_OPCODE_RDID = 0x9f,
    NOR_OPCODE_WRSR = 0x01,
    NOR_OPCODE_RDSR = 0x05,
    NOR_OPCODE_WREN = 0x06,
    NOR_OPCODE_READ = 0x03,
    NOR_OPCODE_PROG = 0x02,
    NOR_OPCODE_E4K = 0x20,
    NOR_OPCODE_E32K = 0x52,
    NOR_OPCODE_E64K = 0xd8,
    NOR_OPCODE_ENTER_4B = 0xb7,
    NOR_OPCODE_EXIT_4B = 0xe9,
};

#ifdef __cplusplus
}
#endif// __cplusplus

#endif// __SYS_SPI_NOR_H__