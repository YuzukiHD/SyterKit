/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>
#include <drivers/clk/clk.h>

#include <common.h>
#include <jmp.h>
#include <mmu.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>

#include <drivers/dma/dma.h>
#include <drivers/dram/dram.h>
#include <drivers/spi/spi.h>
#include <dt-compatible/dma-dt.h>
#include <dt-compatible/dram-dt.h>
#include <dt-compatible/spi-dt.h>

#include "lcd.h"
#include "lcd_init.h"

#define CONFIG_HEAP_BASE (0x40800000)
#define CONFIG_HEAP_SIZE (16 * 1024 * 1024)

extern sunxi_serial_t uart_dbg;

static sunxi_dram_t dram;

static sunxi_spi_t sunxi_spi0_lcd;
static gpio_mux_t lcd_dc_pins;
static gpio_mux_t lcd_res_pins;

static void LCD_Set_DC(uint8_t val)
{
	sunxi_gpio_set_value(&lcd_dc_pins, val);
}

static void LCD_Set_RES(uint8_t val)
{
	sunxi_gpio_set_value(&lcd_res_pins, val);
}

static void LCD_Write_Bus(uint8_t dat)
{
	uint8_t tx[1]; /* Transmit buffer */
	int r; /* Return value */

	tx[0] = dat;
	r = sunxi_spi_transfer(&sunxi_spi0_lcd, SPI_IO_SINGLE, tx, 1, 0, 0); /* Perform SPI transfer */
	if (r < 0)
		pr_err("SPI: SPI Xfer error!\n");
}

void LCD_Write_Data_Bus(void *dat, uint32_t len)
{
	int r = sunxi_spi_transfer(&sunxi_spi0_lcd, SPI_IO_SINGLE, dat, len, 0, 0); /* Perform SPI transfer */
	if (r < 0)
		pr_err("SPI: SPI Xfer error!\n");
}

void LCD_WR_DATA(uint16_t dat)
{
	LCD_Write_Bus(dat >> 8);
	LCD_Write_Bus(dat);
}

void LCD_Address_Set(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
	LCD_WR_REG(0x2a);
	LCD_WR_DATA(x1);
	LCD_WR_DATA(x2);
	LCD_WR_REG(0x2b);
	LCD_WR_DATA(y1);
	LCD_WR_DATA(y2);
	LCD_WR_REG(0x2c);
}

void LCD_WR_DATA8(uint8_t dat)
{
	LCD_Write_Bus(dat);
}

void LCD_WR_REG(uint8_t dat)
{
	LCD_Set_DC(0);
	LCD_Write_Bus(dat);
	LCD_Set_DC(1);
}

static void LCD_Init(void)
{
	LCD_Set_RES(0); //复位
	mdelay(100);
	LCD_Set_RES(1);
	mdelay(100);

	LCD_WR_REG(0x11); //Sleep out
	mdelay(120); //Delay 120ms
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
	LCD_WR_DATA8(0x20); //2b

	LCD_WR_REG(0xC0);
	LCD_WR_DATA8(0x2C);

	LCD_WR_REG(0xC2);
	LCD_WR_DATA8(0x01);

	LCD_WR_REG(0xC3);
	LCD_WR_DATA8(0x01);

	LCD_WR_REG(0xC4);
	LCD_WR_DATA8(0x18); //VDV, 0x20:0v

	LCD_WR_REG(0xC6);
	LCD_WR_DATA8(0x13); //0x13:60Hz

	LCD_WR_REG(0xD0);
	LCD_WR_DATA8(0xA4);
	LCD_WR_DATA8(0xA1);

	LCD_WR_REG(0xD6);
	LCD_WR_DATA8(0xA1); //sleep in后，gate输出为GND

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
	LCD_WR_DATA8(0x1D); //使用240根gate  (N+1)*8
	LCD_WR_DATA8(0x00); //设定gate起点位置
	LCD_WR_DATA8(0x00); //当gate没有用完时，bit4(TMG)设为0

	LCD_WR_REG(0x21);

	LCD_WR_REG(0x29);
}

int main(void)
{
	sunxi_dma_t dma;
	int spi_node;

	spi_node = syterkit_dt_alias_node("spi0", SUNXI_SPI_COMPATIBLE);
	if (sunxi_dma_dt_read_alias(&dma, "dma0") != DRIVER_OK || sunxi_spi_dt_read_config(&sunxi_spi0_lcd, spi_node, &dma) != DRIVER_OK ||
	    !sunxi_gpio_dt_read_property(&lcd_dc_pins, spi_node, "allwinner,lcd-dc-gpio") || !sunxi_gpio_dt_read_property(&lcd_res_pins, spi_node, "allwinner,lcd-reset-gpio")) {
		pr_err("SPI LCD: invalid devicetree configuration\n");
		return -1;
	}

	sunxi_clk_init();

	if (sunxi_dram_dt_read_alias(&dram, "dram0") != DRIVER_OK) {
		pr_err("DRAM: invalid devicetree configuration\n");
		return -1;
	}
	uint32_t dram_size = sunxi_dram_init(&dram);
	arm32_mmu_enable(dram.memory_base, dram_size);

	pr_debug("enable mmu ok\n");

	malloc_init(CONFIG_HEAP_BASE, CONFIG_HEAP_SIZE);

	pr_info("Hello World!\n");

	if (sunxi_spi_init(&sunxi_spi0_lcd) != 0) {
		pr_err("SPI: init failed\n");
	}
	sunxi_gpio_init(&lcd_dc_pins);
	sunxi_gpio_init(&lcd_res_pins);

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
