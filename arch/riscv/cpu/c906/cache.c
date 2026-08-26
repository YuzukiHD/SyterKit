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
 * @file cache.c
 * @brief C906 cache-control implementation used by the RISC-V MMU layer.
 *
 * The C906 exposes cache controls through machine CSRs rather than the
 * standard RISC-V cache-management instructions. This file centralizes the
 * required CSR setup and line/all-cache maintenance operations, including
 * the instruction synchronization barriers needed after each operation.
 */

/**
 * @brief Insert a data synchronization barrier.
 *
 * The C906 @c fence.i sequence drains prior instruction effects before code
 * or data is observed through a newly configured cache state.
 */
void data_sync_barrier(void)
{
	asm volatile("fence.i");
}

/**
 * @brief Initialize the cache configuration.
 *
 * The writes select the C906 cache operation mode, enable hit-control
 * features, configure machine status bits, and set implementation-specific
 * prefetch hints. These values are part of the C906 programming model.
 */
void cache_init(void)
{
	csr_write(mcor, 0x70013); // Configure cache options
	csr_write(mhcr, 0x11ff); // Set cache hit control register
	csr_set(mxstatus, 0x638000); // Set machine status register
	csr_write(mhint, 0x16e30c); // Set hint for cache operations
}

/**
 * @brief Enable the data cache.
 *
 * The write sets the C906 data-cache enable field in @c mhcr. Existing cache
 * configuration from ::cache_init is preserved by the hardware register.
 */
void dcache_enable(void)
{
	csr_write(mhcr, 0x2); // Set the data cache enable bit
}

/**
 * @brief Enable the instruction cache.
 *
 * The set operation enables instruction fetches from the C906 instruction
 * cache without disturbing the data-cache control bits.
 */
void icache_enable(void)
{
	csr_set(mhcr, 0x1); // Set the instruction cache enable bit
}

/**
 * @brief Enable the SV39 MMU with cache initialization.
 *
 * Initialization is ordered so cache mode and status are programmed before
 * either cache is enabled. The routine does not install a page table; it only
 * enables the cache controls used with an already configured MMU.
 */
void mmu_enable(void)
{
	cache_init();
	dcache_enable();
	icache_enable();
}

/* The first address is rounded down to a 64-byte line. The operation writes
 * back dirty lines and finishes with sync.i so subsequent instruction/data
 * accesses observe the completed maintenance. */
void flush_dcache_range(uint64_t start, uint64_t end)
{
	register uint64_t i asm("a0") = start & ~(L1_CACHE_BYTES - 1);
	for (; i < end; i += L1_CACHE_BYTES)
		asm volatile("dcache.cpa a0");
	asm volatile("sync.i");
}

/* The range is rounded down to a cache-line boundary and each covered line is
 * invalidated with the C906 physical-address operation before synchronization. */
void invalidate_dcache_range(uint64_t start, uint64_t end)
{
	register uint64_t i asm("a0") = start & ~(L1_CACHE_BYTES - 1);
	for (; i < end; i += L1_CACHE_BYTES)
		asm volatile("dcache.ipa a0");
	asm volatile("sync.i");
}

/**
 * @brief Write back every C906 data-cache line.
 *
 * The @c dcache.call instruction cleans all levels of the data cache. The
 * hardware instruction includes the C906 completion ordering required before
 * another agent or the instruction stream consumes the modified memory.
 */
void flush_dcache_all(void)
{
	asm volatile("dcache.call");
}

/**
 * @brief Invalidate every C906 data-cache line.
 *
 * The @c dcache.ciall operation discards all data-cache lines, so subsequent
 * reads refill from memory rather than observing stale cached contents.
 */
void invalidate_dcache_all(void)
{
	asm volatile("dcache.ciall");
}
