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

#include <sys-spi-nor.h>
#include <sys-spi.h>

static const spi_nor_info_t spi_nor_info_table[] = {
        {"W25X40", 0xef3013, 512 * 1024, 4096, 1, 256, 3, NOR_OPCODE_READ, NOR_OPCODE_PROG, NOR_OPCODE_WREN, NOR_OPCODE_E4K, 0, NOR_OPCODE_E64K, 0},
        {"W25Q128JVEIQ", 0xefc018, 16 * 1024 * 1024, 4096, 1, 256, 3, NOR_OPCODE_READ, NOR_OPCODE_PROG, NOR_OPCODE_WREN, NOR_OPCODE_E4K, NOR_OPCODE_E32K, NOR_OPCODE_E64K, 0},
        {"GD25D10B", 0xc84011, 128 * 1024, 4096, 1, 256, 3, NOR_OPCODE_READ, NOR_OPCODE_PROG, NOR_OPCODE_WREN, NOR_OPCODE_E4K, NOR_OPCODE_E32K, NOR_OPCODE_E64K, 0},
};

static inline int spinor_read_sfdp(sunxi_spi_t *spi, sfdp_t *sfdp) {
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

    if ((sfdp->header.sign[0] != 'S') || (sfdp->header.sign[1] != 'F') ||
        (sfdp->header.sign[2] != 'D') || (sfdp->header.sign[3] != 'P'))
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

static inline int spinor_read_id(sunxi_spi_t *spi, uint32_t *id) {
    uint8_t tx[1];
    uint8_t rx[3];

    tx[0] = NOR_OPCODE_RDID;
    if (!sunxi_spi_transfer(spi, SPI_IO_SINGLE, tx, 1, rx, 3))
        return 0;
    *id = (rx[0] << 16) | (rx[1] << 8) | (rx[2] << 0);
    return 1;
}

static int spi_nor_get_info(sunxi_spi_t *spi, spi_nor_info_t *info) {
    sfdp_t sfdp;
    spi_nor_info_t *tmp_info;
    uint32_t v, i, id;

    spinor_read_id(spi, &id);
    info->id = id;

    if (spinor_read_sfdp(spi, &sfdp)) {
        info->name = "SPDF";
        v = (sfdp.basic_table.table[7] << 24) | (sfdp.basic_table.table[6] << 16) |
            (sfdp.basic_table.table[5] << 8) | (sfdp.basic_table.table[4] << 0);
        if (v & (1 << 31)) {
            v &= 0x7fffffff;
            info->capacity = 1 << (v - 3);
        } else {
            info->capacity = (v + 1) >> 3;
        }
        /* Basic flash parameter table 1th dword */
        v = (sfdp.basic_table.table[3] << 24) | (sfdp.basic_table.table[2] << 16) |
            (sfdp.basic_table.table[1] << 8) | (sfdp.basic_table.table[0] << 0);

        if ((info->capacity <= (16 * 1024 * 1024)) && (((v >> 17) & 0x3) != 0x2))
            info->address_length = 3;
        else
            info->address_length = 4;
        if (((v >> 0) & 0x3) == 0x1)
            info->opcode_erase_4k = (v >> 8) & 0xff;
        else
            info->opcode_erase_4k = 0x00;
        info->opcode_erase_32k = 0x00;
        info->opcode_erase_64k = 0x00;
        info->opcode_erase_256k = 0x00;

        /* Basic flash parameter table 8th dword */
        v = (sfdp.basic_table.table[31] << 24) | (sfdp.basic_table.table[30] << 16) |
            (sfdp.basic_table.table[29] << 8) | (sfdp.basic_table.table[28] << 0);

        switch ((v >> 0) & 0xff) {
            case 12:
                info->opcode_erase_4k = (v >> 8) & 0xff;
                break;
            case 15:
                info->opcode_erase_32k = (v >> 8) & 0xff;
                break;
            case 16:
                info->opcode_erase_64k = (v >> 8) & 0xff;
                break;
            case 18:
                info->opcode_erase_256k = (v >> 8) & 0xff;
                break;
            default:
                break;
        }
        switch ((v >> 16) & 0xff) {
            case 12:
                info->opcode_erase_4k = (v >> 24) & 0xff;
                break;
            case 15:
                info->opcode_erase_32k = (v >> 24) & 0xff;
                break;
            case 16:
                info->opcode_erase_64k = (v >> 24) & 0xff;
                break;
            case 18:
                info->opcode_erase_256k = (v >> 24) & 0xff;
                break;
            default:
                break;
        }

        /* Basic flash parameter table 9th dword */
        v = (sfdp.basic_table.table[35] << 24) | (sfdp.basic_table.table[34] << 16) |
            (sfdp.basic_table.table[33] << 8) | (sfdp.basic_table.table[32] << 0);
        switch ((v >> 0) & 0xff) {
            case 12:
                info->opcode_erase_4k = (v >> 8) & 0xff;
                break;
            case 15:
                info->opcode_erase_32k = (v >> 8) & 0xff;
                break;
            case 16:
                info->opcode_erase_64k = (v >> 8) & 0xff;
                break;
            case 18:
                info->opcode_erase_256k = (v >> 8) & 0xff;
                break;
            default:
                break;
        }
        switch ((v >> 16) & 0xff) {
            case 12:
                info->opcode_erase_4k = (v >> 24) & 0xff;
                break;
            case 15:
                info->opcode_erase_32k = (v >> 24) & 0xff;
                break;
            case 16:
                info->opcode_erase_64k = (v >> 24) & 0xff;
                break;
            case 18:
                info->opcode_erase_256k = (v >> 24) & 0xff;
                break;
            default:
                break;
        }
        if (info->opcode_erase_4k != 0x00)
            info->blksz = 4096;
        else if (info->opcode_erase_32k != 0x00)
            info->blksz = 32768;
        else if (info->opcode_erase_64k != 0x00)
            info->blksz = 65536;
        else if (info->opcode_erase_256k != 0x00)
            info->blksz = 262144;

        info->opcode_write_enable = NOR_OPCODE_WREN;
        info->read_granularity = 1;
        info->opcode_read = NOR_OPCODE_READ;

        if ((sfdp.basic_table.major == 1) && (sfdp.basic_table.minor < 5)) {
            /* Basic flash parameter table 1th dword */
            v = (sfdp.basic_table.table[3] << 24) | (sfdp.basic_table.table[2] << 16) |
                (sfdp.basic_table.table[1] << 8) | (sfdp.basic_table.table[0] << 0);
            if ((v >> 2) & 0x1)
                info->write_granularity = 64;
            else
                info->write_granularity = 1;
        } else if ((sfdp.basic_table.major == 1) && (sfdp.basic_table.minor >= 5)) {
            /* Basic flash parameter table 11th dword */
            v = (sfdp.basic_table.table[43] << 24) | (sfdp.basic_table.table[42] << 16) |
                (sfdp.basic_table.table[41] << 8) | (sfdp.basic_table.table[40] << 0);
            info->write_granularity = 1 << ((v >> 4) & 0xf);
        }
        info->opcode_write = NOR_OPCODE_PROG;
        return 1;
    } else if (spinor_read_id(spi, &id) && (id != 0xffffff) && (id != 0)) {
        info->id = id;
        for (i = 0; i < ARRAY_SIZE(spi_nor_info_table); i++) {
            tmp_info = &spi_nor_info_table[i];
            if (id == tmp_info->id) {
                memcpy(info, tmp_info, sizeof(spi_nor_info_t));
                return 1;
            }
        }
        printk_error("The spi nor flash '0x%x' is not yet supported\r\n", id);
    }
    return 0;
}

int spi_nor_detect(sunxi_spi_t *spi) {
    spi_nor_info_t info;
    spi_nor_get_info(spi, &info);

    return 0;
}
