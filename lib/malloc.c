/* SPDX-License-Identifier: GPL-2.0+ */

#include <malloc.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define MALLOC_ALIGNMENT 16U

struct malloc_block {
	uintptr_t address;
	size_t size;
	size_t requested_size;
	struct malloc_block *next;
};

static struct malloc_block heap_head;
static struct malloc_block heap_tail;

static size_t align_size(size_t size)
{
	if (size > (size_t)-1 - (MALLOC_ALIGNMENT - 1U))
		return 0;

	return (size + MALLOC_ALIGNMENT - 1U) & ~(MALLOC_ALIGNMENT - 1U);
}

static uintptr_t block_limit(const struct malloc_block *block)
{
	if (block == &heap_tail)
		return block->address;

	return block->address - sizeof(*block);
}

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
