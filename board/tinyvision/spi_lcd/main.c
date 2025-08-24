/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>

#include <common.h>
#include <jmp.h>
#include <mmu.h>
#include <smalloc.h>
#include <sstdlib.h>
#include <string.h>

#include "sys-dma.h"
#include "sys-dram.h"
#include "sys-spi.h"

#include "lcd.h"
#include "lcd_init.h"

#define CONFIG_HEAP_BASE (0x40800000)
#define CONFIG_HEAP_SIZE (16 * 1024 * 1024)

extern sunxi_serial_t uart_dbg;

extern dram_para_t dram_para;

extern sunxi_dma_t sunxi_dma;

static sunxi_spi_t sunxi_spi0_lcd = {
		.base = 0x04025000,
		.id = 0,
		.clk_rate = 75 * 1000 * 1000,
		.gpio =
				{
						.gpio_cs = {GPIO_PIN(GPIO_PORTC, 1), GPIO_PERIPH_MUX4},
						.gpio_sck = {GPIO_PIN(GPIO_PORTC, 0), GPIO_PERIPH_MUX4},
						.gpio_mosi = {GPIO_PIN(GPIO_PORTC, 2), GPIO_PERIPH_MUX4},
						.gpio_miso = {GPIO_PIN(GPIO_PORTC, 3), GPIO_PERIPH_MUX4},
				},
		.spi_clk =
				{
						.spi_clock_cfg_base = CCU_BASE + CCU_SPI0_CLK_REG,
						.spi_clock_factor_n_offset = SPI_CLK_SEL_FACTOR_N_OFF,
						.spi_clock_source = SPI_CLK_SEL_PERIPH_300M,
				},
		.parent_clk_reg =
				{
						.rst_reg_base = CCU_BASE + CCU_SPI_BGR_REG,
						.rst_reg_offset = SPI_DEFAULT_CLK_RST_OFFSET(0),
						.gate_reg_base = CCU_BASE + CCU_SPI_BGR_REG,
						.gate_reg_offset = SPI_DEFAULT_CLK_GATE_OFFSET(0),
						.parent_clk = 300000000,
				},
		.dma_handle = &sunxi_dma,
};

static gpio_mux_t lcd_dc_pins = {
		.pin = GPIO_PIN(GPIO_PORTC, 4),
		.mux = GPIO_OUTPUT,
};

static gpio_mux_t lcd_res_pins = {
		.pin = GPIO_PIN(GPIO_PORTC, 5),
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
	int r;		   /* Return value */

	tx[0] = dat;
	r = sunxi_spi_transfer(&sunxi_spi0_lcd, SPI_IO_SINGLE, tx, 1, 0, 0); /* Perform SPI transfer */
	if (r < 0)
		printk_error("SPI: SPI Xfer error!\n");
}

void LCD_Write_Data_Bus(void *dat, uint32_t len) {
	int r = sunxi_spi_transfer(&sunxi_spi0_lcd, SPI_IO_SINGLE, dat, len, 0, 0); /* Perform SPI transfer */
	if (r < 0)
		printk_error("SPI: SPI Xfer error!\n");
}

void LCD_WR_DATA(uint16_t dat) {
	LCD_Write_Bus(dat >> 8);
	LCD_Write_Bus(dat);
}

void LCD_Address_Set(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
	LCD_WR_REG(0x2a);
	LCD_WR_DATA(x1);
	LCD_WR_DATA(x2);
	LCD_WR_REG(0x2b);
	LCD_WR_DATA(y1);
	LCD_WR_DATA(y2);
	LCD_WR_REG(0x2c);
}

void LCD_WR_DATA8(uint8_t dat) {
	LCD_Write_Bus(dat);
}

void LCD_WR_REG(uint8_t dat) {
	LCD_Set_DC(0);
	LCD_Write_Bus(dat);
	LCD_Set_DC(1);
}

static void LCD_Init(void) {
	LCD_Set_RES(0);//复位
	mdelay(100);
	LCD_Set_RES(1);
	mdelay(100);

	LCD_WR_REG(0x11);//Sleep out
	mdelay(120);	 //Delay 120ms
	LCD_WR_REG(0x36);
	LCD_WR_DATA8(0x00);

	LCD_WR_REG(0x3A);
	LCD_WR_DATA8(0x05);

	LCD_WR_REG(0xB2);
	LCD_WR_DATA8(0x1F);
	LCD_WR_DATA8(0x1F);
	LCD_WR_DATA8(0x00);
	LCD_WR_DATA8(0x33);
	LCD_WR_DATA8(0x33);

	LCD_WR_REG(0xB7);
	LCD_WR_DATA8(0x35);

	LCD_WR_REG(0xBB);
	LCD_WR_DATA8(0x20);//2b

	LCD_WR_REG(0xC0);
	LCD_WR_DATA8(0x2C);

	LCD_WR_REG(0xC2);
	LCD_WR_DATA8(0x01);

	LCD_WR_REG(0xC3);
	LCD_WR_DATA8(0x01);

	LCD_WR_REG(0xC4);
	LCD_WR_DATA8(0x18);//VDV, 0x20:0v

	LCD_WR_REG(0xC6);
	LCD_WR_DATA8(0x13);//0x13:60Hz

	LCD_WR_REG(0xD0);
	LCD_WR_DATA8(0xA4);
	LCD_WR_DATA8(0xA1);

	LCD_WR_REG(0xD6);
	LCD_WR_DATA8(0xA1);//sleep in后，gate输出为GND

	LCD_WR_REG(0xE0);
	LCD_WR_DATA8(0xF0);
	LCD_WR_DATA8(0x04);
	LCD_WR_DATA8(0x07);
	LCD_WR_DATA8(0x04);
	LCD_WR_DATA8(0x04);
	LCD_WR_DATA8(0x04);
	LCD_WR_DATA8(0x25);
	LCD_WR_DATA8(0x33);
	LCD_WR_DATA8(0x3C);
	LCD_WR_DATA8(0x36);
	LCD_WR_DATA8(0x14);
	LCD_WR_DATA8(0x12);
	LCD_WR_DATA8(0x29);
	LCD_WR_DATA8(0x30);

	LCD_WR_REG(0xE1);
	LCD_WR_DATA8(0xF0);
	LCD_WR_DATA8(0x02);
	LCD_WR_DATA8(0x04);
	LCD_WR_DATA8(0x05);
	LCD_WR_DATA8(0x05);
	LCD_WR_DATA8(0x21);
	LCD_WR_DATA8(0x25);
	LCD_WR_DATA8(0x32);
	LCD_WR_DATA8(0x3B);
	LCD_WR_DATA8(0x38);
	LCD_WR_DATA8(0x12);
	LCD_WR_DATA8(0x14);
	LCD_WR_DATA8(0x27);
	LCD_WR_DATA8(0x31);

	LCD_WR_REG(0xE4);
	LCD_WR_DATA8(0x1D);//使用240根gate  (N+1)*8
	LCD_WR_DATA8(0x00);//设定gate起点位置
	LCD_WR_DATA8(0x00);//当gate没有用完时，bit4(TMG)设为0

	LCD_WR_REG(0x21);

	LCD_WR_REG(0x29);
}

int main(void) {
	sunxi_serial_init(&uart_dbg);

	sunxi_clk_init();

	uint32_t dram_size = sunxi_dram_init(&dram_para);
	arm32_mmu_enable(SDRAM_BASE, dram_size);

	printk_debug("enable mmu ok\n");

	smalloc_init(CONFIG_HEAP_BASE, CONFIG_HEAP_SIZE);

	printk_info("Hello World!\n");

	sunxi_gpio_init(lcd_dc_pins.pin, lcd_dc_pins.mux);
	sunxi_gpio_init(lcd_res_pins.pin, lcd_res_pins.mux);

	if (sunxi_spi_init(&sunxi_spi0_lcd) != 0) {
		printk_error("SPI: init failed\n");
	}

	LCD_Init();

	LCD_Fill_All(WHITE);

	LCD_ShowString(0, 40, "LCD_W:", RED, WHITE, 16, 0);

	LCD_ShowIntNum(48, 40, LCD_W, 3, RED, WHITE, 16);

	LCD_ShowString(80, 40, "LCD_H:", RED, WHITE, 16, 0);

	LCD_ShowIntNum(128, 40, LCD_H, 3, RED, WHITE, 16);

	LCD_ShowString(80, 40, "LCD_H:", RED, WHITE, 16, 0);

	LCD_ShowString(0, 80, "LCD ST7789V2", BLUE, WHITE, 32, 0);

	LCD_ShowString(0, 160, "SyterKit", BLACK, WHITE, 32, 0);

	LCD_ShowString(0, 240, "1.0.2", BLACK, WHITE, 32, 0);

	sunxi_spi_disable(&sunxi_spi0_lcd);

	arm32_mmu_disable();

	clean_syterkit_data();

	sunxi_clk_reset();

	jmp_to_fel();

	return 0;
}