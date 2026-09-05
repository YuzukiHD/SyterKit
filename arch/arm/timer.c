/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file timer.c
 * @brief ARM architectural counter timebase and delay functions.
 *
 * The Allwinner ARM timer is exposed through CNTPCT at a fixed 24 MHz rate.
 * Public time values are derived from that counter without interrupts.
 */

#include <io.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>

#include <timer.h>

static uint32_t init_timestamp = 0;

/** @brief Record the current microsecond counter as the log epoch. */
void set_timer_count()
{
	init_timestamp = (uint32_t)time_us();
}

/*
 * 64bit arch timer.CNTPCT
 * Freq = 24000000Hz
 */
/*
 * @brief Read the 64-bit ARM architectural physical counter.
 * @return Monotonic counter value in 24-MHz ticks.
 */
uint64_t get_arch_counter(void)
{
	uint32_t low = 0, high = 0;
	asm volatile("mrrc p15, 0, %0, %1, c14" : "=r"(low), "=r"(high) : : "memory");
	return ((uint64_t)high << 32) | (uint64_t)low;
}

/*
 * @brief Return elapsed milliseconds from the architectural counter.
 * @return Elapsed milliseconds at the fixed 24-MHz timer frequency.
 */
uint32_t time_ms(void)
{
	return get_arch_counter() / 24000;
}

/*
 * @brief Return elapsed microseconds from the architectural counter.
 * @return Elapsed microseconds at the fixed 24-MHz timer frequency.
 */
uint64_t time_us(void)
{
	return get_arch_counter() / (uint64_t)24;
}

/*
 * @brief Busy-wait for a number of microseconds.
 * @param[in] us Delay duration in microseconds.
 */
void udelay(uint32_t us)
{
	uint64_t now;

	now = time_us();
	while (time_us() - now < us) {
	};
}

/*
 * @brief Busy-wait for a number of milliseconds.
 * @param[in] ms Delay duration in milliseconds.
 */
void mdelay(uint32_t ms)
{
	uint32_t now;

	now = time_ms();
	while (time_ms() - now < ms) {
	};
}

/* Busy-wait for an implementation-defined loop count. */
void sdelay(uint32_t loops)
{
	__asm__ volatile("1:\n" // Define label 1
			 "subs %0, %1, #1\n" // Subtract 1 from the loop count and store the result in the first operand
			 // If the loop count has become 0, exit the loop
			 "bne 1b" // Jump to label 1, i.e., the beginning of the loop
			 : "=r"(loops) // Output operand: update the loop count in the variable 'loops'
			 : "0"(loops) // Input operand: initialize the second operand with the value of 'loops'
			 : // No other registers are used or modified
	);
}

/*
 * @brief Return the timestamp captured by the last timer initialization.
 * @return Initialization timestamp in microseconds.
 */
uint32_t get_init_timestamp()
{
	return init_timestamp;
}
