#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>

#include <common.h>

#include <reg-ncat.h>
#include <sys-clk.h>

#include <mmu.h>

#include <sys-dma.h>
#include <sys-dram.h>
#include <sys-gpio.h>
#include <sys-i2c.h>
#include <sys-sdcard.h>
#include <sys-spi.h>
#include <sys-uart.h>

sunxi_serial_t uart_dbg = {
        .base = SUNXI_UART0_BASE,
        .id = 0,
        .baud_rate = UART_BAUDRATE_115200,
        .dlen = UART_DLEN_8,
        .stop = UART_STOP_BIT_0,
        .parity = UART_PARITY_NO,
        .gpio_pin = {
                .gpio_tx = {GPIO_PIN(GPIO_PORTD, 22), GPIO_PERIPH_MUX3},
                .gpio_rx = {GPIO_PIN(GPIO_PORTD, 23), GPIO_PERIPH_MUX3},
        },
        .uart_clk = {
                .gate_reg_base = SUNXI_CCU_APP_BASE + BUS_CLK_GATING0_REG,
                .gate_reg_offset = BUS_CLK_GATING0_REG_UART0_PCLK_EN_OFFSET,
                .rst_reg_base = SUNXI_CCU_APP_BASE + BUS_Reset0_REG,
                .rst_reg_offset = BUS_Reset0_REG_PRESETN_UART0_SW_OFFSET,
                .parent_clk = 192000000,
        },
};

sunxi_dma_t sunxi_dma = {
        .dma_reg_base = SUNXI_DMA_BASE,
        .bus_clk = {
                .gate_reg_base = SUNXI_CCU_APP_BASE + BUS_CLK_GATING2_REG,
                .gate_reg_offset = BUS_CLK_GATING2_REG_SGDMA_MCLK_EN_OFFSET,
        },
        .dma_clk = {
                .rst_reg_base = SUNXI_CCU_APP_BASE + BUS_Reset0_REG,
                .rst_reg_offset = BUS_Reset0_REG_HRESETN_SGDMA_SW_OFFSET,
                .gate_reg_base = SUNXI_CCU_APP_BASE + BUS_CLK_GATING0_REG,
                .gate_reg_offset = BUS_CLK_GATING0_REG_SGDMA_HCLK_EN_OFFSET,
        },
};

sunxi_spi_t sunxi_spi0 = {
        .base = SUNXI_SPI0_BASE,
        .id = 0,
        .clk_rate = 25 * 1000 * 1000,
        .gpio = {
                .gpio_cs = {GPIO_PIN(GPIO_PORTC, 10), GPIO_PERIPH_MUX3},
                .gpio_sck = {GPIO_PIN(GPIO_PORTC, 9), GPIO_PERIPH_MUX3},
                .gpio_mosi = {GPIO_PIN(GPIO_PORTC, 8), GPIO_PERIPH_MUX3},
                .gpio_miso = {GPIO_PIN(GPIO_PORTC, 11), GPIO_PERIPH_MUX3},
                .gpio_wp = {GPIO_PIN(GPIO_PORTC, 6), GPIO_PERIPH_MUX3},
                .gpio_hold = {GPIO_PIN(GPIO_PORTC, 7), GPIO_PERIPH_MUX3},
        },
        .spi_clk = {
                .spi_clock_cfg_base = SUNXI_CCU_APP_BASE + SPI_CLK_REG,
                .spi_clock_factor_n_offset = SPI_CLK_REG_SPI_SCLK_DIV2_OFFSET,
                .spi_clock_source = SPI_CLK_REG_SPI_SCLK_SEL_PERI_307M,
        },
        .parent_clk_reg = {
                .rst_reg_base = SUNXI_CCU_APP_BASE + BUS_Reset1_REG,
                .rst_reg_offset = BUS_Reset1_REG_HRESETN_SPI_SW_OFFSET,
                .gate_reg_base = SUNXI_CCU_APP_BASE + BUS_CLK_GATING1_REG,
                .gate_reg_offset = BUS_CLK_GATING1_REG_SPI_HCLK_EN_OFFSET,
                .parent_clk = 307200000,
        },
        .dma_handle = &sunxi_dma,
};

sunxi_i2c_t sunxi_i2c0 = {
        .base = SUNXI_TWI0_BASE,
        .id = SUNXI_I2C0,
        .speed = SUNXI_I2C_SPEED_400K,
        .gpio = {
                .gpio_scl = {GPIO_PIN(GPIO_PORTA, 3), GPIO_PERIPH_MUX4},
                .gpio_sda = {GPIO_PIN(GPIO_PORTA, 4), GPIO_PERIPH_MUX4},
        },
        .i2c_clk = {
                .gate_reg_base = SUNXI_CCU_APP_BASE + BUS_CLK_GATING0_REG,
                .gate_reg_offset = BUS_CLK_GATING0_REG_TWI0_PCLK_EN_OFFSET,
                .rst_reg_base = SUNXI_CCU_APP_BASE + BUS_Reset0_REG,
                .rst_reg_offset = BUS_Reset0_REG_PRESETN_TWI0_SW_OFFSET,
                .parent_clk = 192000000,
        },
};

dram_para_t dram_para = {
        .dram_clk = 528,
        .dram_type = 2,
        .dram_zq = 0x7b7bf9,
        .dram_odt_en = 0x00,
        .dram_para1 = 0x000000d2,
        .dram_para2 = 0x00400000,
        .dram_mr0 = 0x00000E73,
        .dram_mr1 = 0x02,
        .dram_mr2 = 0x0,
        .dram_mr3 = 0x0,
        .dram_tpr0 = 0x00471992,
        .dram_tpr1 = 0x0131A10C,
        .dram_tpr2 = 0x00057041,
        .dram_tpr3 = 0xB4787896,
        .dram_tpr4 = 0x0,
        .dram_tpr5 = 0x48484848,
        .dram_tpr6 = 0x48,
        .dram_tpr7 = 0x1621121e,
        .dram_tpr8 = 0x0,
        .dram_tpr9 = 0x0,
        .dram_tpr10 = 0x00000000,
        .dram_tpr11 = 0x00000000,
        .dram_tpr12 = 0x00000000,
        .dram_tpr13 = 0x34000100,
};

void show_chip() {
    uint32_t chip_sid[4];
    chip_sid[0] = read32(SUNXI_SID_SRAM_BASE + 0x0);
    chip_sid[1] = read32(SUNXI_SID_SRAM_BASE + 0x4);
    chip_sid[2] = read32(SUNXI_SID_SRAM_BASE + 0x8);
    chip_sid[3] = read32(SUNXI_SID_SRAM_BASE + 0xc);

    printk_info("Model: AvaotaSBC Avaota CAM board.\n");
    printk_info("Core: XuanTie E907 RISC-V Core.\n");
    printk_info("Chip SID = %08x%08x%08x%08x\n", chip_sid[0], chip_sid[1], chip_sid[2], chip_sid[3]);
}