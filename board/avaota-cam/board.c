#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>

#include <common.h>

#include <reg-ncat.h>
#include <sys-clk.h>

#include <mmu.h>

#include <sys-gpio.h>
#include <sys-sdcard.h>
#include <sys-dram.h>
#include <sys-spi.h>
#include <sys-uart.h>

sunxi_serial_t uart_dbg = {
        .base = SUNXI_UART0_BASE,
        .id = 0,
        .gpio_tx = {GPIO_PIN(GPIO_PORTL, 4), GPIO_PERIPH_MUX3},
        .gpio_rx = {GPIO_PIN(GPIO_PORTL, 5), GPIO_PERIPH_MUX3},
};

dram_para_t dram_para = {
        .dram_clk = 792,
        .dram_type = 3,
        .dram_zq = 0x7b7bfb,
        .dram_odt_en = 0x01,
        .dram_para1 = 0x000010d2,
        .dram_para2 = 0,
        .dram_mr0 = 0x1c70,
        .dram_mr1 = 0x42,
        .dram_mr2 = 0x18,
        .dram_mr3 = 0,
        .dram_tpr0 = 0x004a2195,
        .dram_tpr1 = 0x02423190,
        .dram_tpr2 = 0x0008b061,
        .dram_tpr3 = 0xb4787896,// unused
        .dram_tpr4 = 0,
        .dram_tpr5 = 0x48484848,
        .dram_tpr6 = 0x00000048,
        .dram_tpr7 = 0x1620121e,// unused
        .dram_tpr8 = 0,
        .dram_tpr9 = 0,// clock?
        .dram_tpr10 = 0,
        .dram_tpr11 = 0x00770000,
        .dram_tpr12 = 0x00000002,
        .dram_tpr13 = 0x34050100,
};
