/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __IMAGE_LOADER_H__
#define __IMAGE_LOADER_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#define LINUX_ZIMAGE_MAGIC 0x016f2818

/* Linux zImage Header */
typedef struct {
	uint32_t code[9];
	uint32_t magic;
	uint32_t start;
	uint32_t end;
} linux_zimage_header_t;

int zImage_loader(uint8_t *addr, uint32_t *entry);

int bImage_loader(uint8_t *addr, uint32_t *entry);

int uImage_loader(uint8_t *addr, uint32_t *entry);

#endif// __IMAGE_LOADER_H__