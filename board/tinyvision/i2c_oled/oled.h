#ifndef __OLED_H
#define __OLED_H

/* 发送一个字节 
 * mode:数据/命令标志 0,表示命令;1,表示数据;
 */
void OLED_WR_Byte(uint8_t dat, uint8_t mode);

/* Pow 函数 */
uint32_t OLED_Pow(uint8_t m, uint8_t n);

/* 反显函数 */
void OLED_ColorTurn(uint8_t i);

/* 屏幕旋转180度 */
void OLED_DisplayTurn(uint8_t i);

/* 更新显存到OLED */
void OLED_Refresh(void);

/* OLED 清屏 */
void OLED_Clear(void);

/* 画点 
 * x: 0~127
 * y: 0~63
 * t: 1 填充 0,清空	
 */
void OLED_DrawPoint(uint8_t x, uint8_t y, uint8_t t);

/* 画线
 * x1, y1: 起点坐标
 * x2, y2: 结束坐标
 */
void OLED_DrawLine(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t mode);

/* 画圆
 * x, y: 圆心坐标
 * r: 圆的半径
 */

void OLED_DrawCircle(uint8_t x, uint8_t y, uint8_t r);

/* 在指定位置显示一个字符,包括部分字符
 * x: 0 ~ 127
 * y: 0 ~ 63
 * size1: 选择字体 6x8/6x12/8x16/12x24
 * mode: 0,反色显示; 1,正常显示
 */
void OLED_ShowChar(uint8_t x, uint8_t y, uint8_t chr, uint8_t size1, uint8_t mode);

/* 显示字符串
 * x: 0 ~ 127
 * y: 0 ~ 63
 * *chr: 字符串起始地址 
 * size1: 选择字体 6x8/6x12/8x16/12x24
 * mode: 0,反色显示; 1,正常显示
 */
void OLED_ShowString(uint8_t x, uint8_t y, uint8_t *chr, uint8_t size1, uint8_t mode);

/* 显示数字
 * x: 0 ~ 127
 * y: 0 ~ 63
 * num: 要显示的数字 
 * len: 数字的位数
 * size1: 选择字体 6x8/6x12/8x16/12x24
 * mode: 0,反色显示; 1,正常显示
 */
void OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size1, uint8_t mode);

/* 初始化 OLED */
void OLED_Init(void);

#endif