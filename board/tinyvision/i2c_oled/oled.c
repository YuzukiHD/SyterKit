#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>

#include <common.h>

#include "sys-i2c.h"

#include "oledfont.h"

#define OLED_IIC_ADDR 0x3c
#define OLED_IIC_GPIO_PORT 0

#define OLED_CMD 0	/*写命令 */
#define OLED_DATA 1 /* 写数据 */

uint8_t OLED_GRAM[144][8]; /* 显存 */

sunxi_i2c_t i2c_0 = {
		.base = 0x02502000,
		.id = SUNXI_I2C0,
		.speed = SUNXI_I2C_SPEED_400K,
		.gpio =
				{
						.gpio_scl = {GPIO_PIN(GPIO_PORTE, 4), GPIO_PERIPH_MUX8},
						.gpio_sda = {GPIO_PIN(GPIO_PORTE, 5), GPIO_PERIPH_MUX8},
				},
		.i2c_clk =
				{
						.gate_reg_base = CCU_BASE + CCU_TWI_BGR_REG,
						.gate_reg_offset = TWI_DEFAULT_CLK_GATE_OFFSET(0),
						.rst_reg_base = CCU_BASE + CCU_TWI_BGR_REG,
						.rst_reg_offset = TWI_DEFAULT_CLK_RST_OFFSET(0),
						.parent_clk = 24000000,
				},
};

void OLED_WR_Byte(uint8_t dat, uint8_t mode) {
	if (mode)
		sunxi_i2c_write(&i2c_0, OLED_IIC_ADDR, 0x40, dat);
	else
		sunxi_i2c_write(&i2c_0, OLED_IIC_ADDR, 0x00, dat);
}

uint32_t OLED_Pow(uint8_t m, uint8_t n) {
	uint32_t result = 1;
	while (n--) result *= m;
	return result;
}

void OLED_ColorTurn(uint8_t i) {
	if (i == 0) {
		OLED_WR_Byte(0xA6, OLED_CMD); /* 正常显示 */
	}
	if (i == 1) {
		OLED_WR_Byte(0xA7, OLED_CMD); /* 反色显示 */
	}
}

void OLED_DisplayTurn(uint8_t i) {
	if (i == 0) {
		OLED_WR_Byte(0xC8, OLED_CMD); /* 正常显示 */
		OLED_WR_Byte(0xA1, OLED_CMD);
	}
	if (i == 1) {
		OLED_WR_Byte(0xC0, OLED_CMD); /* 反转显示 */
		OLED_WR_Byte(0xA0, OLED_CMD);
	}
}

void OLED_Refresh(void) {
	for (int i = 0; i < 8; i++) {
		OLED_WR_Byte(0xb0 + i, OLED_CMD);//设置行起始地址
		OLED_WR_Byte(0x00, OLED_CMD);	 //设置低列起始地址
		OLED_WR_Byte(0x10, OLED_CMD);	 //设置高列起始地址
		OLED_WR_Byte(0x78, OLED_DATA);
		OLED_WR_Byte(0x40, OLED_DATA);
		for (int n = 0; n < 128; n++) { OLED_WR_Byte(OLED_GRAM[n][i], OLED_DATA); }
	}
}

void OLED_Clear(void) {
	for (int i = 0; i < 8; i++) {
		OLED_WR_Byte(0xb0 + i, OLED_CMD);//设置页地址（0~7）
		OLED_WR_Byte(0x00, OLED_CMD);	 //设置显示位置—列低地址
		OLED_WR_Byte(0x10, OLED_CMD);	 //设置显示位置—列高地址
		for (int n = 0; n < 128; n++) OLED_GRAM[n][i] = 0;
		OLED_Refresh();//刷新GRAM内容
	}
}

void OLED_DrawPoint(uint8_t x, uint8_t y, uint8_t t) {
	uint8_t i, m, n;
	i = y / 8;
	m = y % 8;
	n = 1 << m;
	if (t) {
		OLED_GRAM[x][i] |= n;
	} else {
		OLED_GRAM[x][i] = ~OLED_GRAM[x][i];
		OLED_GRAM[x][i] |= n;
		OLED_GRAM[x][i] = ~OLED_GRAM[x][i];
	}
}

void OLED_DrawLine(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t mode) {
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
		delta_y = -delta_x;
	}
	if (delta_x > delta_y)
		distance = delta_x;//选取基本增量坐标轴
	else
		distance = delta_y;
	for (uint16_t t = 0; t < distance + 1; t++) {
		OLED_DrawPoint(uRow, uCol, mode);//画点
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

void OLED_DrawCircle(uint8_t x, uint8_t y, uint8_t r) {
	int a, b, num;
	a = 0;
	b = r;
	while (2 * b * b >= r * r) {
		OLED_DrawPoint(x + a, y - b, 1);
		OLED_DrawPoint(x - a, y - b, 1);
		OLED_DrawPoint(x - a, y + b, 1);
		OLED_DrawPoint(x + a, y + b, 1);

		OLED_DrawPoint(x + b, y + a, 1);
		OLED_DrawPoint(x + b, y - a, 1);
		OLED_DrawPoint(x - b, y - a, 1);
		OLED_DrawPoint(x - b, y + a, 1);

		a++;
		num = (a * a + b * b) - r * r;//计算画的点离圆心的距离
		if (num > 0) {
			b--;
			a--;
		}
	}
}

void OLED_ShowChar(uint8_t x, uint8_t y, uint8_t chr, uint8_t size1, uint8_t mode) {
	uint8_t temp, size2, chr1;
	uint8_t x0 = x, y0 = y;
	if (size1 == 8)
		size2 = 6;
	else
		size2 = (size1 / 8 + ((size1 % 8) ? 1 : 0)) * (size1 / 2);//得到字体一个字符对应点阵集所占的字节数
	chr1 = chr - ' ';											  //计算偏移后的值
	for (uint8_t i = 0; i < size2; i++) {
		if (size1 == 8) {
			temp = asc2_0806[chr1][i];
		}//调用0806字体
		else if (size1 == 12) {
			temp = asc2_1206[chr1][i];
		}//调用1206字体
		else if (size1 == 16) {
			temp = asc2_1608[chr1][i];
		}//调用1608字体
		else if (size1 == 24) {
			temp = asc2_2412[chr1][i];
		}//调用2412字体
		else
			return;
		for (uint8_t m = 0; m < 8; m++) {
			if (temp & 0x01)
				OLED_DrawPoint(x, y, mode);
			else
				OLED_DrawPoint(x, y, !mode);
			temp >>= 1;
			y++;
		}
		x++;
		if ((size1 != 8) && ((x - x0) == size1 / 2)) {
			x = x0;
			y0 = y0 + 8;
		}
		y = y0;
	}
}

void OLED_ShowString(uint8_t x, uint8_t y, uint8_t *chr, uint8_t size1, uint8_t mode) {
	while ((*chr >= ' ') && (*chr <= '~'))//判断是不是非法字符!
	{
		OLED_ShowChar(x, y, *chr, size1, mode);
		if (size1 == 8)
			x += 6;
		else
			x += size1 / 2;
		chr++;
	}
}

void OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size1, uint8_t mode) {
	uint8_t temp, m = 0;
	if (size1 == 8)
		m = 2;
	for (uint8_t t = 0; t < len; t++) {
		temp = (num / OLED_Pow(10, len - t - 1)) % 10;
		if (temp == 0) {
			OLED_ShowChar(x + (size1 / 2 + m) * t, y, '0', size1, mode);
		} else {
			OLED_ShowChar(x + (size1 / 2 + m) * t, y, temp + '0', size1, mode);
		}
	}
}

void OLED_Set_Pos(uint8_t x, uint8_t y) {
	OLED_WR_Byte(0xb0 + y, OLED_CMD);
	OLED_WR_Byte(((x & 0xf0) >> 4) | 0x10, OLED_CMD);
	OLED_WR_Byte((x & 0x0f), OLED_CMD);
}

void OLED_Init(void) {
	sunxi_i2c_init(&i2c_0);// Init I2C

	OLED_WR_Byte(0xAE, OLED_CMD);//--turn off oled panel
	OLED_WR_Byte(0x00, OLED_CMD);//---set low column address
	OLED_WR_Byte(0x10, OLED_CMD);//---set high column address
	OLED_WR_Byte(0x40, OLED_CMD);//--set start line address  Set Mapping RAM Display Start Line (0x00~0x3F)
	OLED_WR_Byte(0x81, OLED_CMD);//--set contrast control register
	OLED_WR_Byte(0xCF, OLED_CMD);// Set SEG Output Current Brightness
	OLED_WR_Byte(0xA1, OLED_CMD);//--Set SEG/Column Mapping     0xa0左右反置 0xa1正常
	OLED_WR_Byte(0xC8, OLED_CMD);//Set COM/Row Scan Direction   0xc0上下反置 0xc8正常
	OLED_WR_Byte(0xA6, OLED_CMD);//--set normal display
	OLED_WR_Byte(0xA8, OLED_CMD);//--set multiplex ratio(1 to 64)
	OLED_WR_Byte(0x3f, OLED_CMD);//--1/64 duty
	OLED_WR_Byte(0xD3, OLED_CMD);//-set display offset Shift Mapping RAM Counter (0x00~0x3F)
	OLED_WR_Byte(0x00, OLED_CMD);//-not offset
	OLED_WR_Byte(0xd5, OLED_CMD);//--set display clock divide ratio/oscillator frequency
	OLED_WR_Byte(0x80, OLED_CMD);//--set divide ratio, Set Clock as 100 Frames/Sec
	OLED_WR_Byte(0xD9, OLED_CMD);//--set pre-charge period
	OLED_WR_Byte(0xF1, OLED_CMD);//Set Pre-Charge as 15 Clocks & Discharge as 1 Clock
	OLED_WR_Byte(0xDA, OLED_CMD);//--set com pins hardware configuration
	OLED_WR_Byte(0x12, OLED_CMD);
	OLED_WR_Byte(0xDB, OLED_CMD);//--set vcomh
	OLED_WR_Byte(0x40, OLED_CMD);//Set VCOM Deselect Level
	OLED_WR_Byte(0x20, OLED_CMD);//-Set Page Addressing Mode (0x00/0x01/0x02)
	OLED_WR_Byte(0x02, OLED_CMD);//
	OLED_WR_Byte(0x8D, OLED_CMD);//--set Charge Pump enable/disable
	OLED_WR_Byte(0x14, OLED_CMD);//--set(0x10) disable
	OLED_WR_Byte(0xA4, OLED_CMD);// Disable Entire Display On (0xa4/0xa5)
	OLED_WR_Byte(0xA6, OLED_CMD);// Disable Inverse Display On (0xa6/a7)
	OLED_Clear();
	OLED_WR_Byte(0xAF, OLED_CMD); /*display ON*/
}