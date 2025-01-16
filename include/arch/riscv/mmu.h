/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __MMU_H__
#define __MMU_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include <csr.h>
#include <timer.h>

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
void data_sync_barrier(void);

/**
 * @brief Initialize the cache configuration.
 *
 * This function configures the cache settings by writing specific 
 * values to the control and status registers.
 */
void cache_init(void);

/**
 * @brief Enable the data cache.
 *
 * This function enables the data cache by writing to the machine 
 * cache control register.
 */
void dcache_enable(void);

/**
 * @brief Enable the instruction cache.
 *
 * This function enables the instruction cache by setting the 
 * appropriate control bits in the machine cache control register.
 */
void icache_enable(void);

/**
 * @brief Enable the SV39 MMU with cache initialization.
 *
 * This function initializes the cache and enables both data and
 * instruction caches for the SV39 memory management unit.
 */
void mmu_enable(void);

/**
 * @brief Flush a range of the data cache.
 *
 * This function flushes the data cache for a specified range,
 * ensuring that any dirty cache lines are written back to memory.
 *
 * @param start The starting address of the range to flush.
 * @param end The ending address of the range to flush.
 */
void flush_dcache_range(uint64_t start, uint64_t end);

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
void invalidate_dcache_range(uint64_t start, uint64_t end);

#ifdef __cplusplus
}
#endif

#endif /* __MMU_H__ */
