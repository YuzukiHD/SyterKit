/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __SMALLOC_H__
#define __SMALLOC_H__

#include <stdint.h>
#include <types.h>

#ifdef __cplusplus
extern "C" {
#endif// __cplusplus

struct alloc_struct_t {
	phys_addr_t address;
	uint32_t size;
	uint32_t o_size;
	struct alloc_struct_t *next;
};

#define BYTE_ALIGN(x) (((x + 15) / 16) * 16)

/**
 * Initialize the simple malloc library with the specified heap parameters.
 *
 * @param p_heap_head The starting address of the heap.
 * @param n_heap_size The size of the heap in bytes.
 * @return Zero if successful, non-zero otherwise.
 */
int32_t smalloc_init(uint32_t p_heap_head, uint32_t n_heap_size);

/**
 * Allocate a block of memory from the heap with the specified size.
 *
 * @param num_bytes The number of bytes to allocate.
 * @return A pointer to the allocated memory block, or NULL if allocation fails.
 */
void *smalloc(uint32_t num_bytes);

/**
 * Reallocate a block of memory with the specified new size.
 *
 * @param p The pointer to the memory block to reallocate.
 * @param num_bytes The new size in bytes.
 * @return A pointer to the reallocated memory block, or NULL if reallocation fails.
 */
void *srealloc(void *p, uint32_t num_bytes);

/**
 * Free the memory block pointed to by the specified pointer.
 *
 * @param p The pointer to the memory block to free.
 */
void sfree(void *p);

#ifdef __cplusplus
}
#endif// __cplusplus

#endif// __SMALLOC_H__