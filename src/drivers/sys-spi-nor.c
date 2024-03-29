/* SPDX-License-Identifier: Apache-2.0 */

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

#include <sys-spi.h>
#include <sys-spi-nor.h>

static const spi_nor_info_t spi_nor_info_table[] = {
        {"W25X40", 0xef3013, 512 * 1024, 4096, 1, 256, 3, NOR_OPCODE_READ, NOR_OPCODE_PROG, NOR_OPCODE_WREN, NOR_OPCODE_E4K, 0, NOR_OPCODE_E64K, 0},
        {"W25Q128JVEIQ", 0xefc018, 16 * 1024 * 1024, 4096, 1, 256, 3, NOR_OPCODE_READ, NOR_OPCODE_PROG, NOR_OPCODE_WREN, NOR_OPCODE_E4K, NOR_OPCODE_E32K, NOR_OPCODE_E64K, 0},
        {"GD25D10B", 0xc84011, 128 * 1024, 4096, 1, 256, 3, NOR_OPCODE_READ, NOR_OPCODE_PROG, NOR_OPCODE_WREN, NOR_OPCODE_E4K, NOR_OPCODE_E32K, NOR_OPCODE_E64K, 0},
};

