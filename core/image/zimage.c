/**
 * @file zimage.c
 * @brief Linux zImage header validation and entry-point resolution.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>

#include <image/image_loader.h>

/**
 * @brief Validate a Linux zImage and calculate its execution address.
 * @param[in] addr Address of the loaded zImage header.
 * @param[out] entry Receives @p addr plus the header's start offset.
 * @return Zero when the Linux magic matches, or -1 for an unsupported image.
 */
int zImage_loader(uint8_t *addr, uint32_t *entry)
{
	linux_zimage_header_t *zimage_header = (linux_zimage_header_t *)addr;

	printk_debug("Linux zImage->magic = 0x%x\n", zimage_header->magic);
	printk_debug("Linux zImage->start = 0x%x\n", (uint32_t)(uintptr_t)addr + zimage_header->start);
	printk_debug("Linux zImage->end   = 0x%x\n", (uint32_t)(uintptr_t)addr + zimage_header->end);

	if (zimage_header->magic == LINUX_ZIMAGE_MAGIC) {
		*entry = (uint32_t)(uintptr_t)addr + zimage_header->start;
		return 0;
	}

	printk_error("unsupported kernel image\n");

	return -1;
}
