/* SPDX-License-Identifier: GPL-2.0+ */

#include <drivers/serial/serial.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <config.h>
#include <log.h>
#include <drivers/clk/clk.h>
#include <timer.h>

#include <common.h>
#include <jmp.h>
#include <mmu.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>

#include <cli/cli.h>
#include <cli/cli_shell.h>
#include <cli/cli_termesc.h>

#include <dt-bindings/soc/sun55iw3.h>
#include <drivers/dram/dram.h>
#include <dt-compatible/dram-dt.h>
#include <drivers/i2c/i2c.h>
#include <drivers/remoteproc/remoteproc.h>
#include <drivers/rtc/rtc.h>
#include <drivers/mmc/sdcard.h>
#include <drivers/sid/sid.h>
#include <drivers/spi/spi.h>

#include <drivers/pmu/axp.h>
#include <dt-compatible/i2c-dt.h>
#include <dt-compatible/remoteproc-dt.h>
#include <dt-compatible/spi-dt.h>

#include <fdt_wrapper.h>
#include <lib/fatfs/ff.h>
#include <drivers/mmc/sdhci.h>
#include <uart.h>

static sunxi_dram_t dram;

#define CONFIG_HEAP_BASE (0x40800000)
#define CONFIG_HEAP_SIZE (16 * 1024 * 1024)

extern sunxi_serial_t uart_dbg;

extern void set_rpio_power_mode(void);

static sunxi_spi_t sunxi_spi0_lcd;
/* A1 LCD wiring is fixed; only the SPI controller is read from DT. */
static gpio_mux_t lcd_dc_pins = {
	.base = SUNXI_R_GPIO_BASE,
	.pin = GPIO_PIN(GPIO_PORTL, 13),
	.bank = 0,
	.mux = GPIO_OUTPUT,
};
static gpio_mux_t lcd_res_pins = {
	.base = SUNXI_R_GPIO_BASE,
	.pin = GPIO_PIN(GPIO_PORTL, 9),
	.bank = 0,
	.mux = GPIO_OUTPUT,
};
static gpio_mux_t lcd_blk_pins = {
	.base = SUNXI_R_GPIO_BASE,
	.pin = GPIO_PIN(GPIO_PORTL, 8),
	.bank = 0,
	.mux = GPIO_OUTPUT,
};

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

	tx[0] = dat;
	int r = sunxi_spi_transfer(&sunxi_spi0_lcd, SPI_IO_SINGLE, tx, 1, 0, 0); /* Perform SPI transfer */
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

void LCD_Address_Set(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
	LCD_WR_REG(0x2a);
	LCD_WR_DATA(x1 + 52);
	LCD_WR_DATA(x2 + 52);
	LCD_WR_REG(0x2b);
	LCD_WR_DATA(y1 + 40);
	LCD_WR_DATA(y2 + 40);
	LCD_WR_REG(0x2c);
}

static void LCD_Init(void)
{
	LCD_Set_RES(0); //复位
	mdelay(100);
	LCD_Set_RES(1);
	mdelay(100);

	LCD_WR_REG(0x11);
	mdelay(120);
	LCD_WR_REG(0x36);
	LCD_WR_DATA8(0x00);

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
}

#define LCD_W 135
#define LCD_H 240

void LCD_Fill_All(uint16_t color)
{
	LCD_Address_Set(0, 0, LCD_W - 1, LCD_H - 1); // 设置显示范围
	uint16_t *video_mem = malloc(LCD_W * LCD_H);

	for (uint32_t i = 0; i < LCD_W * LCD_H; i++) {
		video_mem[i] = color;
	}

	LCD_Write_Data_Bus(video_mem, LCD_W * LCD_H * (sizeof(uint16_t) / sizeof(uint8_t)));

	free(video_mem);
}

int main(void)
{
	axp_pmu_t axp2202;
	axp_pmu_t axp1530;
	sunxi_i2c_t i2c;
	sunxi_remoteproc_t e906;

	if (sunxi_serial_init_stdout() != 0)
		return -1;

	show_banner();
	if (sunxi_remoteproc_dt_read_alias(&e906, "e906", NULL) != DRIVER_OK) {
		pr_err("RISC-V E906: invalid devicetree configuration\n");
		return -1;
	}
	if (sunxi_i2c_dt_read_alias(&i2c, "i2c0") != DRIVER_OK || pmu_axp2202_config(&axp2202, &i2c) != DRIVER_OK ||
	    pmu_axp1530_config(&axp1530, &i2c) != DRIVER_OK || sunxi_spi_dt_read_alias(&sunxi_spi0_lcd, "spi-lcd", NULL) != DRIVER_OK) {
		pr_err("Board: invalid devicetree configuration\n");
		return -1;
	}

	sunxi_clk_init();

	sunxi_clk_dump();

	set_rpio_power_mode();

	sunxi_i2c_init(&i2c);

	pmu_axp2202_init(&axp2202);

	pmu_axp1530_init(&axp1530);

	pmu_axp2202_set_vol(&axp2202, "dcdc1", 1100, 1);

	pmu_axp1530_set_dual_phase(&axp1530);
	pmu_axp1530_set_vol(&axp1530, "dcdc1", 1100, 1);
	pmu_axp1530_set_vol(&axp1530, "dcdc2", 1100, 1);

	pmu_axp2202_set_vol(&axp2202, "dcdc2", 920, 1);
	pmu_axp2202_set_vol(&axp2202, "dcdc3", 1160, 1);
	pmu_axp2202_set_vol(&axp2202, "dcdc4", 3300, 1);

	pmu_axp2202_dump(&axp2202);
	pmu_axp1530_dump(&axp1530);

	if (sunxi_remoteproc_reset(&e906) != DRIVER_OK) {
		pr_err("RISC-V E906: reset failed\n");
		return -1;
	}

	/* Initialize the DRAM and enable memory management unit (MMU). */
	if (sunxi_dram_dt_read_alias(&dram, "dram0") != DRIVER_OK) {
		pr_err("DRAM: invalid devicetree configuration\n");
		return -1;
	}
	uint32_t dram_size = sunxi_dram_init(&dram);

	sunxi_clk_dump();

	arm32_mmu_enable(dram.memory_base, dram_size);

	/* Initialize the small memory allocator. */
	malloc_init(CONFIG_HEAP_BASE, CONFIG_HEAP_SIZE);

	sunxi_nsi_init();

	sunxi_clk_dump();

	sunxi_gpio_init(&lcd_dc_pins);
	sunxi_gpio_init(&lcd_res_pins);
	sunxi_gpio_init(&lcd_blk_pins);

	if (sunxi_spi_init(&sunxi_spi0_lcd) != 0) {
		pr_err("SPI: init failed\n");
		return -1;
	}

	LCD_Init();

	pr_err("SPI LCD done\n");

	LCD_Fill_All(0xFFFF);

	sunxi_gpio_set_value(&lcd_blk_pins, 1);

	mdelay(100);

	while (1) {
		pr_err("SPI LCD done\n");
		mdelay(10000);
	}

	return 0;
}
