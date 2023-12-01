/* SPDX-License-Identifier: Apache-2.0 */

#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <types.h>
#include <io.h>

#include <log.h>

#include "sys-timer.h"

/*
 * 64bit arch timer.CNTPCT
 * Freq = 24000000Hz
 */
uint64_t get_arch_counter(void)
{
	uint32_t low = 0, high = 0;
	asm volatile("mrrc p15, 0, %0, %1, c14" : "=r"(low), "=r"(high) : : "memory");
	return ((uint64_t)high << 32) | (uint64_t)low;
}

/*
 * get current time.(millisecond)
 */
uint32_t time_ms(void)
{
	return get_arch_counter() / 24000;
}

/*
 * get current time.(microsecond)
 */
uint64_t time_us(void)
{
	return get_arch_counter() / (uint64_t)24;
}

void udelay(uint64_t us)
{
	uint64_t now;

	now = time_us();
	while (time_us() - now < us)
		{};
}

void mdelay(uint32_t ms)
{
	udelay(ms * 1000);
	uint32_t now;

	now = time_ms();
	while (time_ms() - now < ms) 
		{};
}

void sdelay(uint32_t loops)
{
	__asm__ volatile("1:\n"
					 "subs %0, %1, #1\n"
					 "bne 1b"
					 : "=r"(loops)
					 : "0"(loops));
}
