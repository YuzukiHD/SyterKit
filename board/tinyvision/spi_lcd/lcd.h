/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __LCD_H__
#define __LCD_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#define LCD_W 240
#define LCD_H 240

void LCD_Fill(uint16_t xsta, uint16_t ysta, uint16_t xend, uint16_t yend, uint16_t color);//指定区域填充颜色

void LCD_DrawPoint(uint16_t x, uint16_t y, uint16_t color);//在指定位置画一个点

void LCD_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);//在指定位置画一条线

void LCD_DrawRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);//在指定位置画一个矩形

void Draw_Circle(uint16_t x0, uint16_t y0, uint8_t r, uint16_t color);//在指定位置画一个圆

void LCD_ShowChar(uint16_t x, uint16_t y, uint8_t num, uint16_t fc, uint16_t bc, uint8_t sizey, uint8_t mode);//显示一个字符

void LCD_ShowString(uint16_t x, uint16_t y, const uint8_t *p, uint16_t fc, uint16_t bc, uint8_t sizey, uint8_t mode);//显示字符串

u32 mypow(uint8_t m, uint8_t n);//求幂

void LCD_ShowIntNum(uint16_t x, uint16_t y, uint16_t num, uint8_t len, uint16_t fc, uint16_t bc, uint8_t sizey);//显示整数变量

void LCD_ShowPicture(uint16_t x, uint16_t y, uint16_t length, uint16_t width, const uint8_t pic[]);//显示图片

//画笔颜色
#define WHITE 0xFFFF
#define BLACK 0x0000
#define BLUE 0x001F
#define BRED 0XF81F
#define GRED 0XFFE0
#define GBLUE 0X07FF
#define RED 0xF800
#define MAGENTA 0xF81F
#define GREEN 0x07E0
#define CYAN 0x7FFF
#define YELLOW 0xFFE0
#define BROWN 0XBC40	 //棕色
#define BRRED 0XFC07	 //棕红色
#define GRAY 0X8430		 //灰色
#define DARKBLUE 0X01CF	 //深蓝色
#define LIGHTBLUE 0X7D7C //浅蓝色
#define GRAYBLUE 0X5458	 //灰蓝色
#define LIGHTGREEN 0X841F//浅绿色
#define LGRAY 0XC618	 //浅灰色(PANNEL),窗体背景色
#define LGRAYBLUE 0XA651 //浅灰蓝色(中间层颜色)
#define LBBLUE 0X2B12	 //浅棕蓝色(选择条目的反色)

#endif//__LCD_H__