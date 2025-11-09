/**
 * @file cache.h
 * @brief Cache control functions for RISC-V architecture.
 *
 * This header file provides functions for initializing and controlling
 * data and instruction caches on RISC-V architecture.
 */
/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __CACHE_H__
#define __CACHE_H__

#include <stdint.h>
#include "barrier.h"
#include "csr.h"

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

/**
 * @brief Flushes the entire data cache.
 *
 * This function flushes all data cache lines, ensuring that any modified or "dirty"
 * cache lines are written back to the main memory. It ensures that the data in the cache
 * is coherent with the memory.
 */
void flush_dcache_all();

/**
 * @brief Invalidates the entire data cache.
 *
 * This function invalidates all data cache lines, ensuring that no stale or outdated
 * data remains in the cache. This operation discards the cache contents and ensures that
 * the next access will fetch fresh data from memory.
 */
void invalidate_dcache_all();

#endif /* __CACHE_H__ */