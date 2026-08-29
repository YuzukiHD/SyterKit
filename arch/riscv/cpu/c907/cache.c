/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file cache.c
 * @brief C907 L1 cache control.
 *
 * C907 uses the T-Head C9xx cache CSRs and address-based maintenance
 * instructions.  Cache maintenance is done purely with the T-Head cache
 * instructions (dcache.call/iall/cpa/ipa) plus a sync.is barrier, matching
 * Allwinner's spl-2.0 c9xx_cache.c.  The memory-mapped L2 controller is
 * deliberately not used: it is not wired up on this SoC, and polling its
 * status register spins forever.
 */

#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <cache.h>
#include <csr.h>

#define L1_CACHE_BYTES (64U)

/**
 * @brief Insert the C907 instruction/data synchronization barrier.
 */
void data_sync_barrier(void)
{
	asm volatile("sync.is" ::: "memory");
}

/**
 * @brief Initialize C907 cache controls and prefetch settings.
 */
void cache_init(void)
{
	csr_write(mcor, 0x70013);
	csr_write(mhcr, 0x11ff);
	csr_set(mxstatus, 0x638000);
	csr_write(mhint, 0x16e30c);
}

/**
 * @brief Enable the C907 data cache.
 */
void dcache_enable(void)
{
	csr_set(mhcr, 1U << 1);
}

/**
 * @brief Disable the C907 data cache.
 */
void dcache_disable(void)
{
	csr_clear(mhcr, MHCR_DE);
}

/**
 * @brief Enable the C907 instruction cache.
 */
void icache_enable(void)
{
	csr_set(mhcr, 1U << 0);
}

/**
 * @brief Initialize the C907 cache interface without enabling an MMU.
 */
void mmu_enable(void)
{
	cache_init();
	dcache_enable();
	icache_enable();
}

/* Clean a C907 L1 cache range. The range is inclusive start / exclusive end.
 * Doxygen documentation lives on the declaration in arch/riscv/include/cache.h. */
void flush_dcache_range(uint64_t start, uint64_t end)
{
	uintptr_t address = (uintptr_t)start & ~(L1_CACHE_BYTES - 1U);
	uintptr_t limit = (uintptr_t)end;

	for (; address < limit; address += L1_CACHE_BYTES)
		asm volatile("dcache.cpa %0" : : "r"(address) : "memory");
	data_sync_barrier();
}

/* Invalidate a C907 L1 cache range. The range is inclusive start / exclusive end.
 * Doxygen documentation lives on the declaration in arch/riscv/include/cache.h. */
void invalidate_dcache_range(uint64_t start, uint64_t end)
{
	uintptr_t address = (uintptr_t)start & ~(L1_CACHE_BYTES - 1U);
	uintptr_t limit = (uintptr_t)end;

	for (; address < limit; address += L1_CACHE_BYTES)
		asm volatile("dcache.ipa %0" : : "r"(address) : "memory");
	data_sync_barrier();
}

/**
 * @brief Clean all C907 L1 data-cache lines.
 */
void flush_dcache_all(void)
{
	asm volatile("dcache.call" ::: "memory");
	data_sync_barrier();
}

/**
 * @brief Invalidate all C907 L1 data-cache lines.
 */
void invalidate_dcache_all(void)
{
	asm volatile("dcache.iall" ::: "memory");
	data_sync_barrier();
}
