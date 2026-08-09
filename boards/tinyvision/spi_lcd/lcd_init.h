/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __LCD_INIT_H__
#define __LCD_INIT_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

void LCD_Write_Data_Bus(void *dat, uint32_t len);

void LCD_WR_DATA8(uint8_t dat);

void LCD_WR_REG(uint8_t dat);

void LCD_WR_DATA(uint16_t dat);

void LCD_Address_Set(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);

#endif//__LCD_INIT_H__