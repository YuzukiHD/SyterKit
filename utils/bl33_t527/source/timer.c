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

// Stub function for handling signal raising
int raise(int signum) {
	return 0;// Always return 0 to indicate successful handling of the signal
}

// Inline function to retrieve the current value of the architecture-specific counter
static inline uint64_t get_arch_counter(void) {
	uint32_t low = 0, high = 0;
	// Use assembly language to read the architecture-specific counter
	asm volatile("mrrc p15, 0, %0, %1, c14"
				 : "=r"(low), "=r"(high)
				 :
				 : "memory");
	// Combine the low and high values to form a 64-bit counter value
	return (((uint64_t) high) << 32 | low);
}

// Delay function that waits for the specified number of microseconds
static void udelay(uint32_t us) {
	uint64_t t1, t2;

	t1 = get_arch_counter();// Get the current value of the architecture-specific counter
	t2 = t1 + us * 24;		// Calculate the target end time based on the counter frequency (24 MHz)
	do {
		t1 = get_arch_counter();// Continuously update the current time until it reaches the target end time
	} while (t2 >= t1);
}

// Wrapper function for udelay with a different name
void __usdelay(uint32_t loop) {
	udelay(loop);// Call the udelay function to introduce a microsecond delay
}

void mdelay(uint32_t ms) {
	udelay(ms * 1000);
	uint32_t now;

	now = time_ms();
	while (time_ms() - now < ms) {};
}

uint32_t time_ms(void) {
	return get_arch_counter() / 24000;
}

uint64_t time_us(void) {
	return get_arch_counter() / 24;
}
