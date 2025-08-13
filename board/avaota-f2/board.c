/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>

#include <common.h>

#include <reg-ncat.h>
#include <sys-clk.h>

#include <mmu.h>

#include <mmc/sys-sdhci.h>
#include <sys-dma.h>
#include <sys-dram.h>
#include <sys-gpio.h>
#include <sys-i2c.h>
#include <sys-pwm.h>
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
                .gpio_tx = {GPIO_PIN(GPIO_PORTL, 4), GPIO_PERIPH_MUX3},
                .gpio_rx = {GPIO_PIN(GPIO_PORTL, 5), GPIO_PERIPH_MUX3},
        },
        .uart_clk = {
                .gate_reg_base = CCU_BASE + CCU_UART_BGR_REG,
                .gate_reg_offset = SERIAL_DEFAULT_CLK_GATE_OFFSET(0),
                .rst_reg_base = CCU_BASE + CCU_UART_BGR_REG,
                .rst_reg_offset = SERIAL_DEFAULT_CLK_RST_OFFSET(0),
                .parent_clk = SERIAL_DEFAULT_PARENT_CLK,
        },
};

void show_chip() {
    uint32_t chip_sid[4];
    chip_sid[0] = read32(SUNXI_SID_SRAM_BASE + 0x0);
    chip_sid[1] = read32(SUNXI_SID_SRAM_BASE + 0x4);
    chip_sid[2] = read32(SUNXI_SID_SRAM_BASE + 0x8);
    chip_sid[3] = read32(SUNXI_SID_SRAM_BASE + 0xc);

    printk_info("Model: AvaotaSBC Avaota F2 board.\n");
    printk_info("Core: XuanTie E907 RISC-V Core.\n");
    printk_info("Chip SID = %08x%08x%08x%08x\n", chip_sid[0], chip_sid[1], chip_sid[2], chip_sid[3]);
}