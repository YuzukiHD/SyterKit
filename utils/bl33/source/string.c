/* SPDX-License-Identifier: GPL-2.0+ */

#include <byteorder.h>
#include <endian.h>
#include <io.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <types.h>

extern void *memset0(void *s, int c, size_t count);

void *memset(void *s, int c, size_t count) {
	asm volatile("bx %0"
				 :
				 : "r"(memset0));

	return s;
}

extern void *memcpy0(void *dest, const void *src, size_t n);

void *memcpy(void *dest, const void *src, size_t count) {
	asm volatile("bx %0"
				 :
				 : "r"(memcpy0));

	return dest;
}