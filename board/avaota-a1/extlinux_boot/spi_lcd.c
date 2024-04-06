/* SPDX-License-Identifier: Apache-2.0 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <config.h>
#include <log.h>
#include <timer.h>

#include <common.h>
#include <jmp.h>
#include <mmu.h>
#include <smalloc.h>
#include <sstdlib.h>
#include <string.h>

#include <cli.h>
#include <cli_shell.h>
#include <cli_termesc.h>

#include <reg-ncat.h>
#include <sys-clk.h>
#include <sys-dram.h>
#include <sys-i2c.h>
#include <sys-rtc.h>
#include <sys-sdcard.h>
#include <sys-sid.h>
#include <sys-spi.h>

#include <pmu/axp.h>

#include <fdt_wrapper.h>
#include <ff.h>
#include <sys-sdhci.h>
#include <uart.h>

static sunxi_spi_t sunxi_spi0_lcd = {
        .base = SUNXI_R_SPI_BASE,
        .id = 0,
        .clk_rate = 75 * 1000 * 1000,
        .gpio_cs = {GPIO_PIN(GPIO_PORTL, 10), GPIO_PERIPH_MUX6},
        .gpio_sck = {GPIO_PIN(GPIO_PORTL, 11), GPIO_PERIPH_MUX6},
        .gpio_mosi = {GPIO_PIN(GPIO_PORTL, 12), GPIO_PERIPH_MUX6},
        .clk_reg = {
                .ccu_base = SUNXI_R_PRCM_BASE,
                .spi_clk_reg_offest = SUNXI_S_SPI_CLK_REG,
                .spi_bgr_reg_offset = SUNXI_S_SPI_BGR_REG,
        }};

static gpio_mux_t lcd_dc_pins = {
        .pin = GPIO_PIN(GPIO_PORTL, 13),
        .mux = GPIO_OUTPUT,
};

static gpio_mux_t lcd_res_pins = {
        .pin = GPIO_PIN(GPIO_PORTL, 9),
        .mux = GPIO_OUTPUT,
};

static gpio_mux_t lcd_blk_pins = {
        .pin = GPIO_PIN(GPIO_PORTL, 8),
        .mux = GPIO_OUTPUT,
};

static void LCD_Set_DC(uint8_t val) {
    sunxi_gpio_set_value(lcd_dc_pins.pin, val);
}

static void LCD_Set_RES(uint8_t val) {
    sunxi_gpio_set_value(lcd_res_pins.pin, val);
}

static void LCD_Write_Bus(uint8_t dat) {
    uint8_t tx[1]; /* Transmit buffer */
    tx[0] = dat;
    /* Perform SPI transfer */
    if (sunxi_spi_transfer(&sunxi_spi0_lcd, SPI_IO_SINGLE, tx, 1, 0, 0) < 0)
        printk(LOG_LEVEL_ERROR, "SPI: SPI Xfer error!\n");
}

static void LCD_Write_Data_Bus(void *dat, uint32_t len) {
    if (sunxi_spi_transfer(&sunxi_spi0_lcd, SPI_IO_SINGLE, dat, len, 0, 0) < 0)
        printk(LOG_LEVEL_ERROR, "SPI: SPI Xfer error!\n");
}

static void LCD_WR_DATA(uint16_t dat) {
    LCD_Write_Bus(dat >> 8);
    LCD_Write_Bus(dat);
}

static void LCD_WR_DATA8(uint8_t dat) {
    LCD_Write_Bus(dat);
}

static void LCD_WR_REG(uint8_t dat) {
    LCD_Set_DC(0);
    LCD_Write_Bus(dat);
    LCD_Set_DC(1);
}

static void LCD_Address_Set(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
    LCD_WR_REG(0x2a);
    LCD_WR_DATA(x1 + 40);
    LCD_WR_DATA(x2 + 40);
    LCD_WR_REG(0x2b);
    LCD_WR_DATA(y1 + 52);
    LCD_WR_DATA(y2 + 52);
    LCD_WR_REG(0x2c);
}

static void LCD_Open_BLK() {
    sunxi_gpio_set_value(lcd_blk_pins.pin, 1);
}

#define LCD_W 240
#define LCD_H 135

static void LCD_Fill_All(uint16_t color) {
    uint16_t i, j;
    LCD_Address_Set(0, 0, LCD_W - 1, LCD_H - 1);// 设置显示范围
    uint16_t *video_mem = smalloc(LCD_W * LCD_H);

    for (uint32_t i = 0; i < LCD_W * LCD_H; i++) {
        video_mem[i] = color;
    }

    LCD_Write_Data_Bus(video_mem, LCD_W * LCD_H * (sizeof(uint16_t) / sizeof(uint8_t)));

    sfree(video_mem);
}

static void LCD_Init(void) {
    sunxi_gpio_init(lcd_dc_pins.pin, lcd_dc_pins.mux);
    sunxi_gpio_init(lcd_res_pins.pin, lcd_res_pins.mux);
    sunxi_gpio_init(lcd_blk_pins.pin, lcd_blk_pins.mux);

    dma_init();

    if (sunxi_spi_init(&sunxi_spi0_lcd) != 0) {
        printk(LOG_LEVEL_ERROR, "SPI: init failed\n");
    }

    LCD_Set_RES(0);//复位
    mdelay(100);
    LCD_Set_RES(1);
    mdelay(100);

    LCD_WR_REG(0x11);
    mdelay(120);
    LCD_WR_REG(0x36);
    LCD_WR_DATA8(0xA0);

    LCD_WR_REG(0x3A);
    LCD_WR_DATA8(0x05);

    LCD_WR_REG(0xB2);
    LCD_WR_DATA8(0x0C);
    LCD_WR_DATA8(0x0C);
    LCD_WR_DATA8(0x00);
    LCD_WR_DATA8(0x33);
    LCD_WR_DATA8(0x33);

    LCD_WR_REG(0xB7);
    LCD_WR_DATA8(0x35);

    LCD_WR_REG(0xBB);
    LCD_WR_DATA8(0x19);

    LCD_WR_REG(0xC0);
    LCD_WR_DATA8(0x2C);

    LCD_WR_REG(0xC2);
    LCD_WR_DATA8(0x01);

    LCD_WR_REG(0xC3);
    LCD_WR_DATA8(0x12);

    LCD_WR_REG(0xC4);
    LCD_WR_DATA8(0x20);

    LCD_WR_REG(0xC6);
    LCD_WR_DATA8(0x0F);

    LCD_WR_REG(0xD0);
    LCD_WR_DATA8(0xA4);
    LCD_WR_DATA8(0xA1);

    LCD_WR_REG(0xE0);
    LCD_WR_DATA8(0xD0);
    LCD_WR_DATA8(0x04);
    LCD_WR_DATA8(0x0D);
    LCD_WR_DATA8(0x11);
    LCD_WR_DATA8(0x13);
    LCD_WR_DATA8(0x2B);
    LCD_WR_DATA8(0x3F);
    LCD_WR_DATA8(0x54);
    LCD_WR_DATA8(0x4C);
    LCD_WR_DATA8(0x18);
    LCD_WR_DATA8(0x0D);
    LCD_WR_DATA8(0x0B);
    LCD_WR_DATA8(0x1F);
    LCD_WR_DATA8(0x23);

    LCD_WR_REG(0xE1);
    LCD_WR_DATA8(0xD0);
    LCD_WR_DATA8(0x04);
    LCD_WR_DATA8(0x0C);
    LCD_WR_DATA8(0x11);
    LCD_WR_DATA8(0x13);
    LCD_WR_DATA8(0x2C);
    LCD_WR_DATA8(0x3F);
    LCD_WR_DATA8(0x44);
    LCD_WR_DATA8(0x51);
    LCD_WR_DATA8(0x2F);
    LCD_WR_DATA8(0x1F);
    LCD_WR_DATA8(0x1F);
    LCD_WR_DATA8(0x20);
    LCD_WR_DATA8(0x23);

    LCD_WR_REG(0x21);

    LCD_WR_REG(0x29);

    LCD_Fill_All(0x0000);
}

#define SPLASH_START_X 52
#define SPLASH_START_Y 43
#define SPLASH_W 135
#define SPLASH_H 48

static void LCD_Show_Splash(uint8_t *splash_dest) {
    uint16_t i, j, k = 0;
    LCD_Address_Set(SPLASH_START_X, SPLASH_START_Y, SPLASH_START_X + SPLASH_W - 1, SPLASH_START_Y + SPLASH_H - 1);// 设置显示范围

    uint16_t *video_mem = smalloc(SPLASH_W * SPLASH_H);

    for (i = 0; i < SPLASH_W; i++) {
        for (j = 0; j < SPLASH_H; j++) {
            video_mem[k] = (splash_dest[k * 2] << 8) | splash_dest[k * 2 + 1];
            k++;
        }
    }

    LCD_Write_Data_Bus(video_mem, SPLASH_W * SPLASH_H * (sizeof(uint16_t) / sizeof(uint8_t)));

    sfree(video_mem);
}