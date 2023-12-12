/* SPDX-License-Identifier: Apache-2.0 */

#ifndef __SMALLOC_H__
#define __SMALLOC_H__

#include <stdint.h>
#include <types.h>

struct alloc_struct_t {
    phys_addr_t address;
    uint32_t size;
    uint32_t o_size;
    struct alloc_struct_t *next;
};

#define BYTE_ALIGN(x) (((x + 15) / 16) * 16)

int32_t smalloc_init(uint32_t p_heap_head, uint32_t n_heap_size);

void *smalloc(uint32_t num_bytes);

void *srealloc(void *p, uint32_t num_bytes);

void sfree(void *p);

#endif// __SMALLOC_H__