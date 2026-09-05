/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __MALLOC_H__
#define __MALLOC_H__

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize or add a memory region used by malloc(), realloc(), and free().
 *
 * @param heap_start Start address of the heap.
 * @param heap_size Size of the heap in bytes.
 * @return Zero on success, or -1 if the heap range is invalid.
 */
int malloc_init(uintptr_t heap_start, size_t heap_size);

/**
 * @brief Add another non-overlapping region to the allocator.
 *
 * Existing allocations remain valid. New allocations prefer the most
 * recently added region, which lets DRAM supersede an early SRAM region.
 */
int malloc_add_region(uintptr_t heap_start, size_t heap_size);

/**
 * @brief Initialize an early heap from the unused tail of linked SRAM.
 *
 * Returns zero when the linker supplied a usable SRAM tail, or -1 when the
 * current image has no such region.
 */
int malloc_init_early(void);

/**
 * @brief Allocate a block from the initialized heap.
 *
 * @param size Requested size in bytes.
 * @return A 16-byte-aligned allocation, or NULL if allocation fails.
 */
void *malloc(size_t size);

/**
 * @brief Resize an allocation while preserving its existing contents.
 *
 * @param ptr Allocation to resize, or NULL to allocate a new block.
 * @param size Requested total size in bytes. A zero size frees @p ptr.
 * @return The resized allocation, or NULL if it was freed or resizing failed.
 */
void *realloc(void *ptr, size_t size);

/**
 * @brief Release an allocation back to the initialized heap.
 *
 * @param ptr Allocation to release. NULL is accepted and has no effect.
 */
void free(void *ptr);

#ifdef __cplusplus
}
#endif

#endif
