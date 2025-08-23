#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>

#include "image_loader.h"

int zImage_loader(uint8_t *addr, uint32_t *entry) {
	linux_zimage_header_t *zimage_header = (linux_zimage_header_t *) addr;

	printk_debug("Linux zImage->magic = 0x%x\n", zimage_header->magic);
	printk_debug("Linux zImage->start = 0x%x\n", (uint32_t) addr + zimage_header->start);
	printk_debug("Linux zImage->end   = 0x%x\n", (uint32_t) addr + zimage_header->end);

	if (zimage_header->magic == LINUX_ZIMAGE_MAGIC) {
		*entry = ((uint32_t) addr + zimage_header->start);
		return 0;
	}

	printk_error("unsupported kernel image\n");

	return -1;
}
