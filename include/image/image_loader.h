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

/**
 * @brief Validate a Linux zImage and resolve its entry address.
 * @param[in] addr Address of the loaded zImage.
 * @param[out] entry Resolved kernel entry address.
 * @retval 0 The image header is valid.
 * @retval -1 The image magic is invalid.
 */
int zImage_loader(uint8_t *addr, uint32_t *entry);

/**
 * @brief Inspect an Android boot image.
 * @param[in] addr Address of the loaded boot image.
 * @param[out] entry Storage for a resolved entry address.
 * @return -1 because this image loader is not yet implemented.
 */
int bImage_loader(uint8_t *addr, uint32_t *entry);

/**
 * @brief Resolve the payload entry in a legacy U-Boot uImage.
 * @param[in] addr Address of the loaded uImage.
 * @param[out] entry Address immediately after the 64-byte uImage header.
 * @return 1 after resolving the entry address.
 */
int uImage_loader(uint8_t *addr, uint32_t *entry);

#endif// __IMAGE_LOADER_H__
