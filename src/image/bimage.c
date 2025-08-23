#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>

#include "image_loader.h"

#define ANDR_BOOT_MAGIC "ANDROID!"
#define ANDR_BOOT_MAGIC_SIZE 8
#define ANDR_BOOT_NAME_SIZE 16
#define ANDR_BOOT_ARGS_SIZE 512
#define BOOT_EXTRA_ARGS_SIZE 1024

typedef struct linux_bimage_header {
	char magic[ANDR_BOOT_MAGIC_SIZE];

	uint32_t kernel_size; /* size in bytes */
	uint32_t kernel_addr; /* physical load addr */

	uint32_t ramdisk_size; /* size in bytes */
	uint32_t ramdisk_addr; /* physical load addr */

	uint32_t second_size; /* size in bytes */
	uint32_t second_addr; /* physical load addr */

	uint32_t tags_addr; /* physical addr for kernel tags */
	uint32_t page_size; /* flash page size we assume */
	uint32_t unused;	/* reserved for future expansion: MUST be 0 */

	/* operating system version and security patch level; for
	 * version "A.B.C" and patch level "Y-M-D":
	 * ver = A << 14 | B << 7 | C         (7 bits for each of A, B, C)
	 * lvl = ((Y - 2000) & 127) << 4 | M  (7 bits for Y, 4 bits for M)
	 * os_version = ver << 11 | lvl */
	uint32_t os_version;

	char name[ANDR_BOOT_NAME_SIZE]; /* asciiz product name */

	char cmdline[ANDR_BOOT_ARGS_SIZE];

	uint32_t id[8]; /* timestamp / checksum / sha1 / etc */

	/* Supplemental command line data; kept here to maintain
	 * binary compatibility with older versions of mkbootimg */
	char extra_cmdline[BOOT_EXTRA_ARGS_SIZE];
	uint32_t recovery_dtbo_size;   /* size of recovery dtbo image */
	uint64_t recovery_dtbo_offset; /*physical load addr */
	uint32_t header_size;		   /*size of boot image header in bytes */
	uint32_t dtb_size;
	uint64_t dtb_addr;
} __attribute__((packed)) linux_bimage_header_t;

int bImage_loader(uint8_t *addr, uint32_t *entry) {
	linux_bimage_header_t *image_header = (linux_bimage_header_t *) addr;

	if (!memcmp(image_header->magic, ANDR_BOOT_MAGIC, 8)) {
		printk_debug("[IMG] kernel magic is ok\n");
		printk_debug("[IMG] kernel_size = 0x%x\n", image_header->kernel_size);
		printk_debug("[IMG] ramdisk_size = 0x%x\n", image_header->ramdisk_size);
	} else {
		printk_error("[IMG] kernel 0x%08x magic is error\n", addr);
		return -1;
	}

	return -1;
}