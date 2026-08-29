/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file malloc.c
 * @brief Minimal fixed-region heap allocator.
 *
 * The allocator carves allocations out of a statically described memory
 * region that is registered with malloc_init() early during firmware start.
 * Each live allocation is preceded in memory by a malloc_block descriptor
 * chained on a singly linked list rooted at heap_head.
 */
#include <malloc.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define MALLOC_ALIGNMENT 16U

/** @struct malloc_block
 *  @brief Bookkeeping descriptor for a single heap allocation.
 */
struct malloc_block {
	uintptr_t address; /**< Aligned start of the user payload. */
	size_t size; /**< Aligned size of the allocated region. */
	size_t requested_size; /**< Unaligned size requested by the caller. */
	struct malloc_block *next; /**< Next descriptor in the heap chain. */
};

static struct malloc_block heap_head; /**< Sentinel marking the start of the heap chain. */
static struct malloc_block heap_tail; /**< Sentinel marking the end of the heap chain. */

/**
 * @brief Round a size up to the allocator alignment boundary.
 *
 * @param[in] size Requested size in bytes.
 * @return The aligned size, or zero if @p size is so large that the
 *         alignment arithmetic would overflow.
 */
static size_t align_size(size_t size)
{
	if (size > (size_t)-1 - (MALLOC_ALIGNMENT - 1U))
		return 0;

	return (size + MALLOC_ALIGNMENT - 1U) & ~(MALLOC_ALIGNMENT - 1U);
}

/**
 * @brief Compute the highest usable address before a given block.
 *
 * @param[in] block Block whose lower neighbour is to be measured.
 * @return The address of the payload of @p block minus the descriptor
 *         overhead, or the payload address itself for the heap tail.
 */
static uintptr_t block_limit(const struct malloc_block *block)
{
	if (block == &heap_tail)
		return block->address;

	return block->address - sizeof(*block);
}

/**
 * @brief Locate the descriptor for a previously returned allocation.
 *
 * @param[in] ptr Pointer previously handed out by malloc().
 * @param[out] previous Optional pointer that receives the descriptor
 *             preceding the found block, or NULL if @p previous is NULL.
 * @return The descriptor matching @p ptr, or NULL if @p ptr is not a live
 *         allocation.
 */
static struct malloc_block *find_block(const void *ptr, struct malloc_block **previous)
{
	struct malloc_block *block = &heap_head;

	while (block->next) {
		if (block->next->address == (uintptr_t)ptr) {
			if (previous)
				*previous = block;
			return block->next;
		}
		block = block->next;
	}

	return NULL;
}

/**
 * @brief Register the memory region used by the heap allocator.
 *
 * The region is aligned up to the allocator boundary and described by the
 * heap_head and heap_tail sentinels.  It must be called once before any
 * other allocator routine.
 *
 * @param[in] heap_start Start address of the memory region.
 * @param[in] heap_size Size of the memory region in bytes.
 * @return 0 on success, or -1 if the region is unusable (null, wrapping,
 *         or too small to hold the descriptor).
 */
int malloc_init(uintptr_t heap_start, size_t heap_size)
{
	uintptr_t aligned_start;
	uintptr_t heap_end;
	size_t adjustment;

	if (!heap_start || heap_start > (uintptr_t)-1 - heap_size)
		return -1;

	aligned_start = (heap_start + MALLOC_ALIGNMENT - 1U) & ~(uintptr_t)(MALLOC_ALIGNMENT - 1U);
	if (aligned_start < heap_start)
		return -1;

	adjustment = aligned_start - heap_start;
	if (heap_size <= adjustment + sizeof(struct malloc_block))
		return -1;

	heap_end = heap_start + heap_size;
	heap_head.address = aligned_start;
	heap_head.size = 0;
	heap_head.requested_size = 0;
	heap_head.next = &heap_tail;
	heap_tail.address = heap_end;
	heap_tail.size = 0;
	heap_tail.requested_size = 0;
	heap_tail.next = NULL;

	return 0;
}

/**
 * @brief Allocate a block of memory from the heap.
 *
 * The free space between consecutive blocks is searched for a gap large
 * enough to hold both the descriptor and the aligned payload.  The
 * requested size is rounded up to the allocator alignment.
 *
 * @param[in] size Number of bytes to allocate.
 * @return A pointer to the aligned payload, or NULL if the heap is not
 *         initialised, @p size is zero, or no suitable gap exists.
 */
void *malloc(size_t size)
{
	struct malloc_block *block;
	struct malloc_block *new_block;
	uintptr_t metadata_address;
	uintptr_t limit;
	size_t aligned_size;

	if (!size || !heap_head.next)
		return NULL;

	aligned_size = align_size(size);
	if (!aligned_size)
		return NULL;

	for (block = &heap_head; block->next; block = block->next) {
		if (block->address > (uintptr_t)-1 - block->size)
			return NULL;

		metadata_address = block->address + block->size;
		limit = block_limit(block->next);
		if (metadata_address > limit || limit - metadata_address < sizeof(*new_block) || limit - metadata_address - sizeof(*new_block) < aligned_size)
			continue;

		new_block = (struct malloc_block *)metadata_address;
		new_block->address = metadata_address + sizeof(*new_block);
		new_block->size = aligned_size;
		new_block->requested_size = size;
		new_block->next = block->next;
		block->next = new_block;

		return (void *)new_block->address;
	}

	return NULL;
}

/**
 * @brief Resize an existing allocation, growing or shrinking it in place.
 *
 * If @p ptr is NULL the call behaves as malloc().  If @p size is zero the
 * block is freed and NULL is returned.  When the block cannot grow in place
 * a fresh allocation is made and the contents are copied across.
 *
 * @param[in] ptr Pointer to a live allocation, or NULL.
 * @param[in] size New size in bytes.
 * @return A pointer to the resized payload, or NULL on failure.
 */
void *realloc(void *ptr, size_t size)
{
	struct malloc_block *block;
	uintptr_t limit;
	size_t aligned_size;
	size_t copy_size;
	void *new_ptr;

	if (!ptr)
		return malloc(size);
	if (!size) {
		free(ptr);
		return NULL;
	}

	block = find_block(ptr, NULL);
	if (!block)
		return NULL;

	aligned_size = align_size(size);
	if (!aligned_size)
		return NULL;

	limit = block_limit(block->next);
	if (block->address <= limit && aligned_size <= limit - block->address) {
		block->size = aligned_size;
		block->requested_size = size;
		return ptr;
	}

	new_ptr = malloc(size);
	if (!new_ptr)
		return NULL;

	copy_size = block->requested_size < size ? block->requested_size : size;
	memcpy(new_ptr, ptr, copy_size);
	free(ptr);

	return new_ptr;
}

/**
 * @brief Release a previously allocated block back to the heap.
 *
 * The descriptor is unlinked from the chain, which makes the underlying
 * space available to future allocations.  A NULL pointer is ignored.
 *
 * @param[in] ptr Pointer to a live allocation, or NULL.
 */
void free(void *ptr)
{
	struct malloc_block *previous;
	struct malloc_block *block;

	if (!ptr)
		return;

	block = find_block(ptr, &previous);
	if (block)
		previous->next = block->next;
}
