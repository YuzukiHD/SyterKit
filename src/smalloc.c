/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <types.h>

#include "smalloc.h"

static struct alloc_struct_t boot_heap_head, boot_heap_tail;

int32_t smalloc_init(uint32_t p_heap_head, uint32_t n_heap_size) {
	boot_heap_head.size = boot_heap_tail.size = 0;
	boot_heap_head.address = p_heap_head;
	boot_heap_tail.address = p_heap_head + n_heap_size;
	boot_heap_head.next = &boot_heap_tail;
	boot_heap_tail.next = 0;
	return 0;
}

void *smalloc(uint32_t num_bytes) {
	struct alloc_struct_t *ptr, *newptr;
	uint32_t actual_bytes;

	if (!num_bytes)
		return 0;

	actual_bytes = BYTE_ALIGN(num_bytes);

	ptr = &boot_heap_head;

	while (ptr && ptr->next) {
		if (ptr->next->address >= (ptr->address + ptr->size + 2 * sizeof(struct alloc_struct_t) + actual_bytes)) {
			break;
		}
		ptr = ptr->next;
	}

	if (!ptr->next) {
		return 0;
	}

	newptr = (struct alloc_struct_t *) (ptr->address + ptr->size);
	if (!newptr) {
		return 0;
	}

	newptr->address = ptr->address + ptr->size + sizeof(struct alloc_struct_t);
	newptr->size = actual_bytes;
	newptr->o_size = num_bytes;
	newptr->next = ptr->next;
	ptr->next = newptr;

	return (void *) newptr->address;
}

void *srealloc(void *p, uint32_t num_bytes) {
	struct alloc_struct_t *ptr, *prev;
	void *tmp;
	uint32_t actual_bytes;

	if (!p) {
		return smalloc(num_bytes);
	}
	if (!num_bytes) {
		return p;
	}

	ptr = &boot_heap_head;
	while (ptr && ptr->next) {
		if (ptr->next->address == (phys_addr_t) p)
			break;
		ptr = ptr->next;
	}
	prev = ptr;
	ptr = ptr->next;

	if (!ptr) {
		return 0;
	}

	actual_bytes = BYTE_ALIGN(ptr->o_size + num_bytes);
	if (actual_bytes == ptr->size) {
		return p;
	}

	if (ptr->next->address >= (ptr->address + actual_bytes + 2 * sizeof(struct alloc_struct_t))) {
		ptr->size = actual_bytes;
		ptr->o_size += num_bytes;

		return p;
	}

	tmp = smalloc(actual_bytes);
	if (!tmp) {
		return 0;
	}
	memcpy(tmp, (void *) ptr->address, ptr->size);
	prev->next = ptr->next; /* delete the node which need be released from the memory block chain  */

	return tmp;
}

void sfree(void *p) {
	struct alloc_struct_t *ptr, *prev;

	if (p == NULL)
		return;

	ptr = &boot_heap_head;
	while (ptr && ptr->next) {
		if (ptr->next->address == (phys_addr_t) p)
			break;
		ptr = ptr->next;
	}

	prev = ptr;
	ptr = ptr->next;

	if (!ptr)
		return;

	prev->next = ptr->next;

	return;
}
