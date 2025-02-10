/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <types.h>

#include <barrier.h>
#include <mmu.h>
#include <timer.h>

#include <csr.h>

#include <common.h>
#include <log.h>

#define L1_CACHE_BYTES (32) /**< Size of L1 cache line in bytes. */

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
void cache_init(void) {                                                        // Configure cache options
    csr_write(mhcr, MHCR_WB | MHCR_WA | MHCR_RS | MHCR_BPE | MHCR_BTE);        // Set cache hit control register
    csr_write(mhint, MHINT_D_PLD | MHINT_IWPE | MHINT_AMR_1 | MHINT_PREF_N_16);// Set hint for cache operations
}

/**
 * @brief Enable the data cache.
 *
 * This function enables the data cache by writing to the machine 
 * cache control register.
 */
void dcache_enable(void) {
    csr_write(mhcr, MHCR_DE);// Set the data cache enable bit
}

/**
 * @brief Enable the instruction cache.
 *
 * This function enables the instruction cache by setting the 
 * appropriate control bits in the machine cache control register.
 */
void icache_enable(void) {
    csr_set(mhcr, MHCR_IE);// Set the instruction cache enable bit
}

/**
 * @brief Enable the SV39 MMU with cache initialization.
 *
 * This function initializes the cache and enables both data and
 * instruction caches for the SV39 memory management unit.
 */
void mmu_enable(void) {
    return;
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
    register uint32_t i asm("a0") = start & ~(L1_CACHE_BYTES - 1);
    for (; i < end; i += L1_CACHE_BYTES)
        asm volatile("dcache.cpa a0");
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
    for (; i < end; i += L1_CACHE_BYTES)
        asm volatile("dcache.ipa a0");
    asm volatile("sync.i");
}

/**
 * @brief Flushes the entire data cache.
 *
 * This function flushes all data cache lines, ensuring that any modified or "dirty"
 * cache lines are written back to the main memory. It ensures that the data in the cache
 * is coherent with the memory.
 */
void flush_dcache_all() {
    asm volatile("dcache.call");
}

/**
 * @brief Invalidates the entire data cache.
 *
 * This function invalidates all data cache lines, ensuring that no stale or outdated
 * data remains in the cache. This operation discards the cache contents and ensures that
 * the next access will fetch fresh data from memory.
 */
void invalidate_dcache_all() {
    asm volatile("dcache.ciall");
}
