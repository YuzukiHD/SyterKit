#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>

#include "image_loader.h"

/* Linux zImage Header */
#define LINUX_ZIMAGE_MAGIC 0x016f2818
typedef struct {
    uint32_t code[9];
    uint32_t magic;
    uint32_t start;
    uint32_t end;
} linux_zimage_header_t;

int zImage_loader(uint8_t *addr, uint32_t *entry) {
    linux_zimage_header_t *zimage_header = (linux_zimage_header_t *) addr;

    printk(LOG_LEVEL_INFO, "Linux zImage->code  = 0x");
    for (int i = 0; i < 9; i++) {
        printk(LOG_LEVEL_MUTE, "%x", code[i]);
    }

    printk(LOG_LEVEL_MUTE, "\n");
    printk(LOG_LEVEL_DEBUG, "Linux zImage->magic = 0x%x\n", zimage_header->magic);
    printk(LOG_LEVEL_DEBUG, "Linux zImage->start = 0x%x\n", (uint32_t) addr + zimage_header->start);
    printk(LOG_LEVEL_DEBUG, "Linux zImage->end   = 0x%x\n", (uint32_t) addr + zimage_header->end);

    if (zimage_header->magic == LINUX_ZIMAGE_MAGIC) {
        *entry = ((uint32_t) addr + zimage_header->start);
        return 0;
    }

    printk(LOG_LEVEL_ERROR, "unsupported kernel image\n");

    return -1;
}
