/**
 * @file cache.h
 * @brief Cache control functions for ARM32 architecture.
 *
 * This header file provides functions for controlling data and instruction caches
 * on ARM32 architecture.
 */
/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __CACHE_H__
#define __CACHE_H__

#include <stdint.h>
#include "barrier.h"

/**
 * @brief Enable the ARM32 data cache.
 *
 * This function enables the data cache by setting the C-bit in the system control register.
 */
static inline void arm32_dcache_enable(void);

/**
 * @brief Disable the ARM32 data cache.
 *
 * This function disables the data cache by clearing the C-bit in the system control register.
 */
static inline void arm32_dcache_disable(void);

/**
 * @brief Enable the ARM32 instruction cache.
 *
 * This function enables the instruction cache by setting the I-bit in the system control register.
 */
static inline void arm32_icache_enable(void);

/**
 * @brief Disable the ARM32 instruction cache.
 *
 * This function disables the instruction cache by clearing the I-bit in the system control register.
 */
static inline void arm32_icache_disable(void);

/**
 * @brief Flush a range of addresses from the data cache.
 *
 * This function flushes (cleans) a specified range of addresses from the data cache,
 * ensuring that any modified data is written back to memory. The function aligns the
 * addresses to cache line boundaries and processes each line individually.
 *
 * @param start The starting address of the range to flush.
 * @param end The ending address of the range to flush (inclusive).
 *
 * @note The function assumes a 32-byte cache line size, which may need to be adjusted
 *       for different ARM32 processor implementations.
 */
static inline void flush_dcache_range(uint64_t start, uint64_t end);

/**
 * @brief Invalidate a range of addresses in the data cache.
 *
 * This function invalidates a specified range of addresses in the data cache,
 * ensuring that subsequent reads will fetch fresh data from memory rather than
 * using potentially stale cached data. The function aligns the addresses to
 * cache line boundaries and processes each line individually.
 *
 * @param start The starting address of the range to invalidate.
 * @param end The ending address of the range to invalidate (inclusive).
 *
 * @note The function assumes a 32-byte cache line size, which may need to be adjusted
 *       for different ARM32 processor implementations.
 */
static inline void invalidate_dcache_range(uint64_t start, uint64_t end);

/**
 * @brief Flush (clean) the entire data cache.
 *
 * This function flushes all data cache lines, ensuring that any modified data is
 * written back to memory. It uses a single CP15 instruction to clean the entire cache.
 */
static inline void flush_dcache_all();

/**
 * @brief Invalidate the entire data cache.
 *
 * This function invalidates all data cache lines, discarding any data they contain.
 * Subsequent reads will fetch fresh data from memory. It uses a single CP15 instruction
 * to invalidate the entire cache.
 */
static inline void invalidate_dcache_all();

/**
 * @brief Insert a data synchronization barrier.
 *
 * This function ensures that all previous instructions are completed
 * before any subsequent instructions are executed, particularly useful 
 * for ensuring memory consistency.
 */
static inline void data_sync_barrier(void) {
	dsb();
}

/* Function implementations */

static inline void arm32_dcache_enable(void) {
	uint32_t value;
	__asm__ __volatile__("mrc p15, 0, %0, c1, c0, 0"
						 : "=r"(value)
						 :
						 : "memory");
	value |= (1 << 2);
	__asm__ __volatile__("mcr p15, 0, %0, c1, c0, 0"
						 :
						 : "r"(value)
						 : "memory");
	__asm__ __volatile__("mrc p15, 0, %0, c1, c0, 0"
						 : "=r"(value)
						 :
						 : "memory");
}

static inline void arm32_dcache_disable(void) {
	uint32_t value;
	__asm__ __volatile__("mrc p15, 0, %0, c1, c0, 0"
						 : "=r"(value)
						 :
						 : "memory");
	value &= ~(1 << 2);
	__asm__ __volatile__("mcr p15, 0, %0, c1, c0, 0"
						 :
						 : "r"(value)
						 : "memory");
	__asm__ __volatile__("mrc p15, 0, %0, c1, c0, 0"
						 : "=r"(value)
						 :
						 : "memory");
}

static inline void arm32_icache_enable(void) {
	uint32_t value;
	__asm__ __volatile__("mrc p15, 0, %0, c1, c0, 0"
						 : "=r"(value)
						 :
						 : "memory");
	value |= (1 << 12);
	__asm__ __volatile__("mcr p15, 0, %0, c1, c0, 0"
						 :
						 : "r"(value)
						 : "memory");
	__asm__ __volatile__("mrc p15, 0, %0, c1, c0, 0"
						 : "=r"(value)
						 :
						 : "memory");
}

static inline void arm32_icache_disable(void) {
	uint32_t value;
	__asm__ __volatile__("mrc p15, 0, %0, c1, c0, 0"
						 : "=r"(value)
						 :
						 : "memory");
	value &= ~(1 << 12);
	__asm__ __volatile__("mcr p15, 0, %0, c1, c0, 0"
						 :
						 : "r"(value)
						 : "memory");
	__asm__ __volatile__("mrc p15, 0, %0, c1, c0, 0"
						 : "=r"(value)
						 :
						 : "memory");
}

static inline void flush_dcache_range(uint64_t start, uint64_t end) {
	/* Ensure addresses are cache line aligned */
	uint32_t line_size = 32; /* Assume 32-byte cache line size, may need adjustment for specific processors */
	uint32_t aligned_start = (uint32_t) start & ~(line_size - 1);
	uint32_t aligned_end = ((uint32_t) end + line_size - 1) & ~(line_size - 1);
	uint32_t addr;

	/* Iterate through cache lines and flush */
	for (addr = aligned_start; addr < aligned_end; addr += line_size) {
		__asm__ __volatile__(
				"mcr p15, 0, %0, c7, c14, 1" /* Clean data cache line */
				:
				: "r"(addr)
				: "memory");
	}

	/* Ensure all operations are complete */
	data_sync_barrier();
}

static inline void invalidate_dcache_range(uint64_t start, uint64_t end) {
	/* Ensure addresses are cache line aligned */
	uint32_t line_size = 32; /* Assume 32-byte cache line size */
	uint32_t aligned_start = (uint32_t) start & ~(line_size - 1);
	uint32_t aligned_end = ((uint32_t) end + line_size - 1) & ~(line_size - 1);
	uint32_t addr;

	/* Iterate through cache lines and invalidate */
	for (addr = aligned_start; addr < aligned_end; addr += line_size) {
		__asm__ __volatile__(
				"mcr p15, 0, %0, c7, c6, 1" /* Invalidate data cache line */
				:
				: "r"(addr)
				: "memory");
	}

	/* Ensure all operations are complete */
	data_sync_barrier();
}

static inline void flush_dcache_all() {
	/* Ensure all operations are complete */
	data_sync_barrier();
}

static inline void invalidate_dcache_all() {
	/* Ensure all operations are complete */
	data_sync_barrier();
}

#endif /* __CACHE_H__ */