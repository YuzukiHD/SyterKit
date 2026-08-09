#include <mmu.h>
#include <common.h>
#include <jmp.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>

#include "lcd.h"
#include "lcd_font.h"
#include "lcd_init.h"

/**
 * @brief Fill the entire display with one color.
 *
 * @param color RGB565 color value.
 */
void LCD_Fill_All(uint16_t color) {
	LCD_Address_Set(0, 0, LCD_W - 1, LCD_H - 1);// 设置显示范围
	uint16_t *video_mem = malloc(LCD_W * LCD_H);

	for (uint32_t i = 0; i < LCD_W * LCD_H; i++) { video_mem[i] = color; }

	LCD_Write_Data_Bus(video_mem, LCD_W * LCD_H * (sizeof(uint16_t) / sizeof(uint8_t)));

	free(video_mem);
}

/**
 * @brief Fill a rectangular display region with one color.
 *
 * @param xsta Left coordinate of the region.
 * @param ysta Top coordinate of the region.
 * @param xend Exclusive right coordinate of the region.
 * @param yend Exclusive bottom coordinate of the region.
 * @param color RGB565 color value.
 */
void LCD_Fill(uint16_t xsta, uint16_t ysta, uint16_t xend, uint16_t yend, uint16_t color) {
	uint16_t i, j;
	LCD_Address_Set(xsta, ysta, xend - 1, yend - 1);//设置显示范围
	for (i = ysta; i < yend; i++) {
		for (j = xsta; j < xend; j++) { LCD_WR_DATA(color); }
	}
}

/**
 * @brief Draw a pixel at the specified coordinates.
 *
 * @param x Horizontal coordinate.
 * @param y Vertical coordinate.
 * @param color RGB565 color value.
 */
void LCD_DrawPoint(uint16_t x, uint16_t y, uint16_t color) {
	LCD_Address_Set(x, y, x, y);//设置光标位置
	LCD_WR_DATA(color);
}


/**
 * @brief Draw a line between two points.
 *
 * @param x1 Horizontal coordinate of the first point.
 * @param y1 Vertical coordinate of the first point.
 * @param x2 Horizontal coordinate of the second point.
 * @param y2 Vertical coordinate of the second point.
 * @param color RGB565 color value.
 */
void LCD_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) {
	uint16_t t;
	int xerr = 0, yerr = 0, delta_x, delta_y, distance;
	int incx, incy, uRow, uCol;
	delta_x = x2 - x1;//计算坐标增量
	delta_y = y2 - y1;
	uRow = x1;//画线起点坐标
	uCol = y1;
	if (delta_x > 0)
		incx = 1;//设置单步方向
	else if (delta_x == 0)
		incx = 0;//垂直线
	else {
		incx = -1;
		delta_x = -delta_x;
	}
	if (delta_y > 0)
		incy = 1;
	else if (delta_y == 0)
		incy = 0;//水平线
	else {
		incy = -1;
		delta_y = -delta_y;
	}
	if (delta_x > delta_y)
		distance = delta_x;//选取基本增量坐标轴
	else
		distance = delta_y;
	for (t = 0; t < distance + 1; t++) {
		LCD_DrawPoint(uRow, uCol, color);//画点
		xerr += delta_x;
		yerr += delta_y;
		if (xerr > distance) {
			xerr -= distance;
			uRow += incx;
		}
		if (yerr > distance) {
			yerr -= distance;
			uCol += incy;
		}
	}
}


/**
 * @brief Draw the outline of a rectangle.
 *
 * @param x1 Horizontal coordinate of the first corner.
 * @param y1 Vertical coordinate of the first corner.
 * @param x2 Horizontal coordinate of the opposite corner.
 * @param y2 Vertical coordinate of the opposite corner.
 * @param color RGB565 color value.
 */
void LCD_DrawRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) {
	LCD_DrawLine(x1, y1, x2, y1, color);
	LCD_DrawLine(x1, y1, x1, y2, color);
	LCD_DrawLine(x1, y2, x2, y2, color);
	LCD_DrawLine(x2, y1, x2, y2, color);
}


/**
 * @brief Draw the outline of a circle.
 *
 * @param x0 Horizontal coordinate of the center.
 * @param y0 Vertical coordinate of the center.
 * @param r Circle radius in pixels.
 * @param color RGB565 color value.
 */
void Draw_Circle(uint16_t x0, uint16_t y0, uint8_t r, uint16_t color) {
	int a, b;
	a = 0;
	b = r;
	while (a <= b) {
		LCD_DrawPoint(x0 - b, y0 - a, color);//3
		LCD_DrawPoint(x0 + b, y0 - a, color);//0
		LCD_DrawPoint(x0 - a, y0 + b, color);//1
		LCD_DrawPoint(x0 - a, y0 - b, color);//2
		LCD_DrawPoint(x0 + b, y0 + a, color);//4
		LCD_DrawPoint(x0 + a, y0 - b, color);//5
		LCD_DrawPoint(x0 + a, y0 + b, color);//6
		LCD_DrawPoint(x0 - b, y0 + a, color);//7
		a++;
		if ((a * a + b * b) > (r * r))//判断要画的点是否过远
		{
			b--;
		}
	}
}

/**
 * @brief Draw one character on the display.
 *
 * @param x Horizontal coordinate.
 * @param y Vertical coordinate.
 * @param num Character to draw.
 * @param fc RGB565 foreground color.
 * @param bc RGB565 background color.
 * @param sizey Font height in pixels.
 * @param mode Zero for opaque drawing, or nonzero for transparent drawing.
 */
void LCD_ShowChar(uint16_t x, uint16_t y, uint8_t num, uint16_t fc, uint16_t bc, uint8_t sizey, uint8_t mode) {
	uint8_t temp, sizex, t, m = 0;
	uint16_t i, TypefaceNum;//一个字符所占字节大小
	uint16_t x0 = x;
	sizex = sizey / 2;
	TypefaceNum = (sizex / 8 + ((sizex % 8) ? 1 : 0)) * sizey;
	num = num - ' ';									//得到偏移后的值
	LCD_Address_Set(x, y, x + sizex - 1, y + sizey - 1);//设置光标位置
	for (i = 0; i < TypefaceNum; i++) {
		if (sizey == 12)
			temp = ascii_1206[num][i];//调用6x12字体
		else if (sizey == 16)
			temp = ascii_1608[num][i];//调用8x16字体
		else if (sizey == 24)
			temp = ascii_2412[num][i];//调用12x24字体
		else if (sizey == 32)
			temp = ascii_3216[num][i];//调用16x32字体
		else
			return;
		for (t = 0; t < 8; t++) {
			if (!mode)//非叠加模式
			{
				if (temp & (0x01 << t))
					LCD_WR_DATA(fc);
				else
					LCD_WR_DATA(bc);
				m++;
				if (m % sizex == 0) {
					m = 0;
					break;
				}
			} else//叠加模式
			{
				if (temp & (0x01 << t))
					LCD_DrawPoint(x, y, fc);//画一个点
				x++;
				if ((x - x0) == sizex) {
					x = x0;
					y++;
					break;
				}
			}
		}
	}
}

/**
 * @brief Draw a null-terminated string on the display.
 *
 * @param x Horizontal coordinate.
 * @param y Vertical coordinate.
 * @param str String to draw.
 * @param fc RGB565 foreground color.
 * @param bc RGB565 background color.
 * @param sizey Font height in pixels.
 * @param mode Zero for opaque drawing, or nonzero for transparent drawing.
 */
void LCD_ShowString(uint16_t x, uint16_t y, const char *str, uint16_t fc, uint16_t bc, uint8_t sizey, uint8_t mode) {
	printk_info("LCD: Show String: \"%s\"\n", str);
	while (*str != '\0') {
		LCD_ShowChar(x, y, (uint8_t) *str, fc, bc, sizey, mode);
		x += sizey / 2;
		str++;
	}
}

/**
 * @brief Raise an unsigned integer to an unsigned integer power.
 *
 * @param m Base value.
 * @param n Exponent.
 * @return The value of @p m raised to @p n.
 */
u32 mypow(uint8_t m, uint8_t n) {
	u32 result = 1;
	while (n--) result *= m;
	return result;
}


/**
 * @brief Draw an unsigned integer using a fixed number of digits.
 *
 * @param x Horizontal coordinate.
 * @param y Vertical coordinate.
 * @param num Value to draw.
 * @param len Number of digits to reserve.
 * @param fc RGB565 foreground color.
 * @param bc RGB565 background color.
 * @param sizey Font height in pixels.
 */
void LCD_ShowIntNum(uint16_t x, uint16_t y, uint16_t num, uint8_t len, uint16_t fc, uint16_t bc, uint8_t sizey) {
	printk_info("LCD: Show Number: \"%d\"\n", num);
	uint8_t t, temp;
	uint8_t enshow = 0;
	uint8_t sizex = sizey / 2;
	for (t = 0; t < len; t++) {
		temp = (num / mypow(10, len - t - 1)) % 10;
		if (enshow == 0 && t < (len - 1)) {
			if (temp == 0) {
				LCD_ShowChar(x + t * sizex, y, ' ', fc, bc, sizey, 0);
				continue;
			} else
				enshow = 1;
		}
		LCD_ShowChar(x + t * sizex, y, temp + 48, fc, bc, sizey, 0);
	}
}
