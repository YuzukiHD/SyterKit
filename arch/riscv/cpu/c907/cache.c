/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file cache.c
 * @brief C907 L1 and L2 cache control.
 *
 * C907 uses the T-Head C9xx cache CSRs and address-based maintenance
 * instructions.  The sun252iw2p1 cache hierarchy also has a memory-mapped
 * L2 controller, so whole-cache operations update both cache levels.
 */

#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <cache.h>
#include <csr.h>
#include <io.h>

#define L1_CACHE_BYTES (64U)

#define C907_L2C_BASE	   (0x37fff000U)
#define C907_L2C_STATUS	   (C907_L2C_BASE + 0x08U)
#define C907_L2C_OPERATION (C907_L2C_BASE + 0x18U)

#define C907_L2C_OPERATION_INVALIDATE	    (0x1U << 4)
#define C907_L2C_OPERATION_CLEAN	    (0x2U << 4)
#define C907_L2C_OPERATION_CLEAN_INVALIDATE (0x3U << 4)
#define C907_L2C_OPERATION_ENABLE	    (1U)

static void l2c_operation(uint32_t operation)
{
	writel(operation | C907_L2C_OPERATION_ENABLE, C907_L2C_OPERATION);
	while (readl(C907_L2C_STATUS) != 0U)
		;
}

static void l2c_invalid_all(void)
{
	l2c_operation(C907_L2C_OPERATION_INVALIDATE);
}

static void l2c_clear_all(void)
{
	l2c_operation(C907_L2C_OPERATION_CLEAN);
}

static void l2c_clear_invalid_all(void)
{
	l2c_operation(C907_L2C_OPERATION_CLEAN_INVALIDATE);
}

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

/**
 * @brief Clean a C907 L1/L2 cache range.
 * @param[in] start Inclusive start address.
 * @param[in] end Exclusive end address.
 */
void flush_dcache_range(uint64_t start, uint64_t end)
{
	uintptr_t address = (uintptr_t)start & ~(L1_CACHE_BYTES - 1U);
	uintptr_t limit = (uintptr_t)end;

	for (; address < limit; address += L1_CACHE_BYTES)
		asm volatile("dcache.cpa %0" : : "r"(address) : "memory");
	data_sync_barrier();
	l2c_clear_all();
}

/**
 * @brief Invalidate a C907 L1/L2 cache range.
 * @param[in] start Inclusive start address.
 * @param[in] end Exclusive end address.
 */
void invalidate_dcache_range(uint64_t start, uint64_t end)
{
	uintptr_t address = (uintptr_t)start & ~(L1_CACHE_BYTES - 1U);
	uintptr_t limit = (uintptr_t)end;

	for (; address < limit; address += L1_CACHE_BYTES)
		asm volatile("dcache.ipa %0" : : "r"(address) : "memory");
	data_sync_barrier();
	l2c_invalid_all();
}

/**
 * @brief Clean all C907 L1 and L2 data-cache lines.
 */
void flush_dcache_all(void)
{
	asm volatile("dcache.call" ::: "memory");
	data_sync_barrier();
	l2c_clear_all();
}

/**
 * @brief Invalidate all C907 L1 and L2 data-cache lines.
 */
void invalidate_dcache_all(void)
{
	asm volatile("dcache.iall" ::: "memory");
	data_sync_barrier();
	l2c_invalid_all();
}
