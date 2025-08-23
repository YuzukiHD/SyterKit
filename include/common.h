/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#ifdef __cplusplus
extern "C" {
#endif// __cplusplus

#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))

#define ALIGN(size, align) (((size) + (align) -1) & (~((align) -1)))
#define OF_ALIGN(size) ALIGN(size, 4)

#ifndef NULL
#define NULL 0
#endif

#define FALSE 0
#define TRUE 1

static inline uint32_t swap_uint32(uint32_t data) {
	volatile uint32_t a, b, c, d;

	a = ((data) &0xff000000) >> 24;
	b = ((data) &0x00ff0000) >> 8;
	c = ((data) &0x0000ff00) << 8;
	d = ((data) &0x000000ff) << 24;

	return a | b | c | d;
}

void abort(void);

int raise(int signum);

void show_banner(void);

#ifdef __cplusplus
}
#endif// __cplusplus

#endif// __COMMON_H__