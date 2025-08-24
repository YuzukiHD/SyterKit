/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <types.h>

#include <mmu.h>
#include <timer.h>

#include <csr.h>

#include <common.h>
#include <log.h>

#define L1_CACHE_BYTES (64) /**< Size of L1 cache line in bytes. */

/**
 * @file mmu.h
 * @brief Memory Management Unit (MMU) interface for cache operations.
 *
 * This header file provides functions and definitions for initializing and 
 * managing the memory management unit (MMU), particularly concerning 
 * data and instruction caches.
 */

/**
 * @brief Insert a data synchronization barrier.
 *
 * This function ensures that all previous instructions are completed
 * before any subsequent instructions are executed, particularly useful 
 * for ensuring memory consistency.
 */
void data_sync_barrier(void) {
	asm volatile("fence.i");
}

/**
 * @brief Initialize the cache configuration.
 *
 * This function configures the cache settings by writing specific 
 * values to the control and status registers.
 */
void cache_init(void) {
	csr_write(mcor, 0x70013);	// Configure cache options
	csr_write(mhcr, 0x11ff);	// Set cache hit control register
	csr_set(mxstatus, 0x638000);// Set machine status register
	csr_write(mhint, 0x16e30c); // Set hint for cache operations
}

/**
 * @brief Enable the data cache.
 *
 * This function enables the data cache by writing to the machine 
 * cache control register.
 */
void dcache_enable(void) {
	csr_write(mhcr, 0x2);// Set the data cache enable bit
}

/**
 * @brief Enable the instruction cache.
 *
 * This function enables the instruction cache by setting the 
 * appropriate control bits in the machine cache control register.
 */
void icache_enable(void) {
	csr_set(mhcr, 0x1);// Set the instruction cache enable bit
}

/**
 * @brief Enable the SV39 MMU with cache initialization.
 *
 * This function initializes the cache and enables both data and
 * instruction caches for the SV39 memory management unit.
 */
void mmu_enable(void) {
	cache_init();
	dcache_enable();
	icache_enable();
}

/**
 * @brief Flush a range of the data cache.
 *
 * This function flushes the data cache for a specified range,
 * ensuring that any dirty cache lines are written back to memory.
 *
 * @param start The starting address of the range to flush.
 * @param end The ending address of the range to flush.
 */
void flush_dcache_range(uint64_t start, uint64_t end) {
	register uint64_t i asm("a0") = start & ~(L1_CACHE_BYTES - 1);
	for (; i < end; i += L1_CACHE_BYTES) asm volatile("dcache.cpa a0");
	asm volatile("sync.i");
}

/**
 * @brief Invalidate a range of the data cache.
 *
 * This function invalidates the data cache for a specified range,
 * ensuring that no stale data remains in the cache for the given
 * addresses.
 *
 * @param start The starting address of the range to invalidate.
 * @param end The ending address of the range to invalidate.
 */
void invalidate_dcache_range(uint64_t start, uint64_t end) {
	register uint64_t i asm("a0") = start & ~(L1_CACHE_BYTES - 1);
	for (; i < end; i += L1_CACHE_BYTES) asm volatile("dcache.ipa a0");
	asm volatile("sync.i");
}