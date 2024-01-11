/* SPDX-License-Identifier: Apache-2.0 */

#ifndef __IMAGE_LOADER_H__
#define __IMAGE_LOADER_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

int zImage_loader(uint8_t *addr, uint32_t *entry);

int bImage_loader(uint8_t *addr, uint32_t *entry);

int uImage_loader(uint8_t *addr, uint32_t *entry);

#endif // __IMAGE_LOADER_H__