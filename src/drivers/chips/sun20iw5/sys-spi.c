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

#define SPI_MOD_CLK 307000000

void sunxi_spi_clk_init(sunxi_spi_t *spi) {
    uint32_t val, reset_offset, gating_offset;

    switch (spi->id) {
        case 0:
            reset_offset = BUS_Reset1_REG_HRESETN_SPI_SW_OFFSET;
            gating_offset = BUS_CLK_GATING1_REG_SPI_HCLK_EN_OFFSET;
            break;
        case 1:
            reset_offset = BUS_Reset1_REG_HRESETN_SPI1_SW_OFFSET;
            gating_offset = BUS_CLK_GATING1_REG_SPI1_HCLK_EN_OFFSET;
            break;
        case 2:
            reset_offset = BUS_Reset1_REG_HRESETN_SPI2_SW_OFFSET;
            gating_offset = BUS_CLK_GATING1_REG_SPI2_HCLK_EN_OFFSET;
            break;
        default:
            reset_offset = BUS_Reset1_REG_HRESETN_SPI_SW_OFFSET;
            gating_offset = BUS_CLK_GATING1_REG_SPI_HCLK_EN_OFFSET;
            break;
    }

    val = (1U << 31) | (0x1 << 24) | (0 << 16) | 0; /* gate enable | use PERIPH_300M */

    printk_trace("SPI: parent_clk=%dMHz\n", SPI_MOD_CLK);

    /* Set SPI CLK Source */
    write32(SUNXI_CCU_APP_BASE + CCU_SPI0_CLK_REG, val);

    /* Deassert spi reset */
    val = read32(SUNXI_CCU_APP_BASE + BUS_Reset1_REG);
    val |= 1 << reset_offset;
    write32(SUNXI_CCU_APP_BASE + CCU_SPI_BGR_REG, val);

    /* Open the spi bus gate */
    val = read32(SUNXI_CCU_APP_BASE + CCU_SPI_BGR_REG);
    val |= 1 << gating_offset;
    write32(SUNXI_CCU_APP_BASE + CCU_SPI_BGR_REG, val);
}