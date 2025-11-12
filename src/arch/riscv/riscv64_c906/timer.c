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
	uint64_t cnt = 0;

	asm volatile("csrr %0, time\n"
				 : "=r"(cnt)
				 :
				 : "memory");

	return cnt;
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
	udelay(ms * 1000);
	uint32_t now;

	now = time_ms();
	while (time_ms() - now < ms) {};
}

void sdelay(uint32_t loops) {
	asm volatile("mv t0, %0\n"// Move the 'loops' value to register t0
				 "1:\n"
				 "addi t0, t0, -1\n"// Decrement t0 by 1
				 "bnez t0, 1b\n"	// Branch back to label 1 if t0 is not zero
				 :					// No output operands
				 : "r"(loops)		// Input operand: loops
				 : "t0"				// Clobbered registers: t0
	);
}

uint32_t get_init_timestamp() {
	return init_timestamp;
}
