/**
 * @file uimage.c
 * @brief Legacy U-Boot uImage entry-point resolver.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>

#include <image/image_loader.h>

#define KERNEL_CODE_OFFSET_IN_UIMAGE 0x40

/**
 * @brief Resolve the payload address immediately after a uImage header.
 * @param[in] addr Address of the loaded 64-byte uImage header.
 * @param[out] entry Receives the address at which the payload starts.
 * @return One after the fixed header offset has been applied.
 */
int uImage_loader(uint8_t *addr, uint32_t *entry)
{
	*entry = (uint32_t)(uintptr_t)(addr + KERNEL_CODE_OFFSET_IN_UIMAGE);
	return 1;
}
