#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>

#include "image_loader.h"

#define KERNEL_CODE_OFFSET_IN_UIMAGE 0x40

int uImage_loader(uint8_t *addr, uint32_t *entry) {
	*entry = (uint32_t) (addr + KERNEL_CODE_OFFSET_IN_UIMAGE);
	return 1;
}