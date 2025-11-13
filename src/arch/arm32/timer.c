/* SPDX-License-Identifier: GPL-2.0+ */

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>

#include <timer.h>

static uint32_t init_timestamp = 0;

void set_timer_count() {
	init_timestamp = (uint32_t) time_us();
}

/*
 * 64bit arch timer.CNTPCT
 * Freq = 24000000Hz
 */
uint64_t get_arch_counter(void) {
	uint32_t low = 0, high = 0;
	asm volatile("mrrc p15, 0, %0, %1, c14"
				 : "=r"(low), "=r"(high)
				 :
				 : "memory");
	return ((uint64_t) high << 32) | (uint64_t) low;
}

/*
 * get current time.(millisecond)
 */
uint32_t time_ms(void) {
	return get_arch_counter() / 24000;
}

/*
 * get current time.(microsecond)
 */
uint64_t time_us(void) {
	return get_arch_counter() / (uint64_t) 24;
}

void udelay(uint32_t us) {
	uint64_t now;

	now = time_us();
	while (time_us() - now < us) {};
}

void mdelay(uint32_t ms) {
	uint32_t now;

	now = time_ms();
	while (time_ms() - now < ms) {};
}

void sdelay(uint32_t loops) {
	__asm__ volatile("1:\n"				// Define label 1
					 "subs %0, %1, #1\n"// Subtract 1 from the loop count and store the result in the first operand
					 // If the loop count has become 0, exit the loop
					 "bne 1b"	  // Jump to label 1, i.e., the beginning of the loop
					 : "=r"(loops)// Output operand: update the loop count in the variable 'loops'
					 : "0"(loops) // Input operand: initialize the second operand with the value of 'loops'
					 :			  // No other registers are used or modified
	);
}

uint32_t get_init_timestamp() {
	return init_timestamp;
}
