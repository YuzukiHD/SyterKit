/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file cache.h
 * @brief ARMv7/AArch32 cache maintenance interface.
 *
 * The low-level helpers perform set/way and MVA operations with the required
 * DSB/ISB barriers. Compatibility wrappers expose the names used by the
 * architecture-neutral cache API.
 */
#ifndef __ARM32_CACHE_H__
#define __ARM32_CACHE_H__

#include <stddef.h>
#include <stdint.h>

#include "barrier.h"

enum {
	ARM32_SCTLR_C = 1U << 2,
	ARM32_SCTLR_I = 1U << 12,
	ARM32_CACHE_LINE_MIN = 16U,
	ARM32_CCSIDR_LINE_SIZE_MASK = 0x7U,
};

/**
 * @brief Read the ARM system-control register.
 * @return Current SCTLR value, including cache enable bits.
 */
static inline __attribute__((always_inline)) uint32_t arm32_cache_read_sctlr(void)
{
	uint32_t value;

	__asm__ __volatile__("mrc p15, 0, %0, c1, c0, 0" : "=r"(value) : : "memory");
	return value;
}

/**
 * @brief Write the ARM system-control register with a read-back barrier.
 * @param[in] value New SCTLR value.
 */
static inline __attribute__((always_inline)) void arm32_cache_write_sctlr(uint32_t value)
{
	__asm__ __volatile__("mcr p15, 0, %0, c1, c0, 0" : : "r"(value) : "memory");
	__asm__ __volatile__("mrc p15, 0, %0, c1, c0, 0" : "=r"(value) : : "memory");
}

/**
 * @brief Read the level-0 data-cache line size from CCSIDR.
 * @return Cache-line size in bytes.
 */
static inline __attribute__((always_inline)) uint32_t arch_dcache_get_cacheline_size(void)
{
	uint32_t csselr = 0;
	uint32_t ccsidr;

	/* Select level 0 data/unified cache before reading CCSIDR. */
	__asm__ __volatile__("mcr p15, 2, %0, c0, c0, 0" : : "r"(csselr));
	isb();
	__asm__ __volatile__("mrc p15, 1, %0, c0, c0, 0" : "=r"(ccsidr));

	return ARM32_CACHE_LINE_MIN << (ccsidr & ARM32_CCSIDR_LINE_SIZE_MASK);
}

/**
 * @brief Clean or invalidate every data-cache set and way.
 * @param[in] invalidate Non-zero to discard lines; zero to clean them to PoC.
 */
static inline __attribute__((always_inline)) void arm32_dcache_maintain_all(uint32_t invalidate)
{
	uint32_t clidr;
	uint32_t level;
	uint32_t loc;

	__asm__ __volatile__("mrc p15, 1, %0, c0, c0, 1" : "=r"(clidr));
	loc = (clidr >> 24) & 0x7U;

	for (level = 0; level < loc; level++) {
		uint32_t cache_type = (clidr >> (level * 3U)) & 0x7U;
		uint32_t csselr;
		uint32_t ccsidr;
		uint32_t line_shift;
		uint32_t ways;
		uint32_t sets;
		uint32_t way_shift;
		uint32_t set;

		/* Skip absent and instruction-only cache levels. */
		if (cache_type < 2U)
			continue;

		csselr = level << 1;
		__asm__ __volatile__("mcr p15, 2, %0, c0, c0, 0" : : "r"(csselr) : "memory");
		isb();
		__asm__ __volatile__("mrc p15, 1, %0, c0, c0, 0" : "=r"(ccsidr));

		line_shift = (ccsidr & ARM32_CCSIDR_LINE_SIZE_MASK) + 4U;
		ways = ((ccsidr >> 3) & 0x3ffU) + 1U;
		sets = ((ccsidr >> 13) & 0x7fffU) + 1U;
		way_shift = ways > 1U ? __builtin_clz(ways - 1U) : 32U;

		for (set = sets; set != 0U; set--) {
			uint32_t way;

			for (way = ways; way != 0U; way--) {
				uint32_t setway = csselr | ((set - 1U) << line_shift);

				if (way > 1U)
					setway |= (way - 1U) << way_shift;
				if (invalidate != 0U) {
					__asm__ __volatile__("mcr p15, 0, %0, c7, c6, 2" : : "r"(setway) : "memory");
				} else {
					__asm__ __volatile__("mcr p15, 0, %0, c7, c14, 2" : : "r"(setway) : "memory");
				}
			}
		}
	}

	/* Restore the L1 data-cache selection for subsequent CCSIDR readers. */
	level = 0;
	__asm__ __volatile__("mcr p15, 2, %0, c0, c0, 0" : : "r"(level) : "memory");
	dsb();
	isb();
}

/**
 * @brief Clean or invalidate data-cache lines covering an address range.
 *
 * Addresses are rounded to the implementation's line size and clipped to
 * the 32-bit ARM address space before issuing MVA maintenance operations.
 *
 * @param[in] start Inclusive first byte address.
 * @param[in] end Exclusive last byte address.
 * @param[in] invalidate Non-zero to invalidate; zero to clean to PoC.
 */
static inline __attribute__((always_inline)) void arm32_dcache_maintain_range(uint64_t start, uint64_t end, uint32_t invalidate)
{
	uint32_t line;
	uint64_t first;
	uint64_t last;
	uint64_t address;

	if (start >= end || start > 0xffffffffULL)
		return;
	if (end > 0x100000000ULL)
		end = 0x100000000ULL;

	line = arch_dcache_get_cacheline_size();
	first = start & ~((uint64_t)line - 1U);
	last = (end + line - 1U) & ~((uint64_t)line - 1U);
	if (last > 0x100000000ULL)
		last = 0x100000000ULL;

	for (address = first; address < last; address += line) {
		uint32_t mva = (uint32_t)address;

		if (invalidate != 0U) {
			/* DCIMVAC: invalidate data cache line by MVA to PoC. */
			__asm__ __volatile__("mcr p15, 0, %0, c7, c6, 1" : : "r"(mva) : "memory");
		} else {
			/* DCCMVAC: clean data cache line by MVA to PoC. */
			__asm__ __volatile__("mcr p15, 0, %0, c7, c10, 1" : : "r"(mva) : "memory");
		}
	}

	dsb();
}

/**
 * @brief Invalidate and enable the ARM data cache.
 *
 * Existing lines are invalidated before setting SCTLR.C, then DSB/ISB make
 * the new state visible to subsequent memory operations.
 */
static inline __attribute__((always_inline)) void arch_dcache_enable(void)
{
	uint32_t reg;

	arm32_dcache_maintain_all(1U);
	reg = arm32_cache_read_sctlr() | ARM32_SCTLR_C;
	arm32_cache_write_sctlr(reg);
	dsb();
	isb();
}

/**
 * @brief Clean and disable the ARM data cache.
 *
 * Cleaning preserves dirty memory before SCTLR.C is cleared and the barrier
 * sequence completes the transition.
 */
static inline __attribute__((always_inline)) void arch_dcache_disable(void)
{
	uint32_t reg;

	arm32_dcache_maintain_all(0U);
	reg = arm32_cache_read_sctlr() & ~ARM32_SCTLR_C;
	arm32_cache_write_sctlr(reg);
	dsb();
	isb();
}

/**
 * @brief Clean a byte range from the ARM data cache.
 * @param[in] start First byte address.
 * @param[in] size Number of bytes in the range.
 */
static inline __attribute__((always_inline)) void arch_dcache_clean_range(unsigned long start, size_t size)
{
	arm32_dcache_maintain_range((uint64_t)start, (uint64_t)start + size, 0U);
}

/**
 * @brief Invalidate a byte range from the ARM data cache.
 * @param[in] start First byte address.
 * @param[in] size Number of bytes in the range.
 */
static inline __attribute__((always_inline)) void arch_dcache_inval_range(unsigned long start, size_t size)
{
	arm32_dcache_maintain_range((uint64_t)start, (uint64_t)start + size, 1U);
}

/** @brief Clean every ARM data-cache line to the point of coherency. */
static inline __attribute__((always_inline)) void arch_flush_dcache_all(void)
{
	arm32_dcache_maintain_all(0U);
}

/** @brief Invalidate every ARM data-cache line. */
static inline __attribute__((always_inline)) void arch_invalidate_dcache_all(void)
{
	arm32_dcache_maintain_all(1U);
}

/** @brief Enable the ARM data cache through the architecture wrapper. */
static inline __attribute__((always_inline)) void arm32_dcache_enable(void)
{
	arch_dcache_enable();
}

/** @brief Disable the ARM data cache through the architecture wrapper. */
static inline __attribute__((always_inline)) void arm32_dcache_disable(void)
{
	arch_dcache_disable();
}

/**
 * @brief Invalidate all ARM instruction-cache lines and branch predictors.
 */
static inline __attribute__((always_inline)) void arm32_icache_invalidate_all(void)
{
	uint32_t zero = 0;

	__asm__ __volatile__("mcr p15, 0, %0, c7, c5, 0" : : "r"(zero) : "memory");
	__asm__ __volatile__("mcr p15, 0, %0, c7, c5, 6" : : "r"(zero) : "memory");
	dsb();
	isb();
}

/**
 * @brief Invalidate and enable the ARM instruction cache.
 */
static inline __attribute__((always_inline)) void arm32_icache_enable(void)
{
	uint32_t reg;

	arm32_icache_invalidate_all();
	reg = arm32_cache_read_sctlr() | ARM32_SCTLR_I;
	arm32_cache_write_sctlr(reg);
	isb();
}

/**
 * @brief Disable and invalidate the ARM instruction cache.
 */
static inline __attribute__((always_inline)) void arm32_icache_disable(void)
{
	uint32_t reg = arm32_cache_read_sctlr() & ~ARM32_SCTLR_I;

	arm32_cache_write_sctlr(reg);
	arm32_icache_invalidate_all();
}

/* Compatibility operations use an exclusive end address. */
static inline __attribute__((always_inline)) void flush_dcache_range(uint64_t start, uint64_t end)
{
	arm32_dcache_maintain_range(start, end, 0U);
}

static inline __attribute__((always_inline)) void invalidate_dcache_range(uint64_t start, uint64_t end)
{
	arm32_dcache_maintain_range(start, end, 1U);
}

static inline __attribute__((always_inline)) void flush_dcache_all(void)
{
	arch_flush_dcache_all();
}

static inline __attribute__((always_inline)) void invalidate_dcache_all(void)
{
	arch_invalidate_dcache_all();
}

/* spl-2.0-compatible names. */
static inline __attribute__((always_inline)) void dcache_enable(void)
{
	arm32_dcache_enable();
}

static inline __attribute__((always_inline)) void dcache_disable(void)
{
	arm32_dcache_disable();
}

static inline __attribute__((always_inline)) void dcache_clean_range(unsigned long start, size_t size)
{
	arch_dcache_clean_range(start, size);
}

static inline __attribute__((always_inline)) void dcache_inval_range(unsigned long start, size_t size)
{
	arch_dcache_inval_range(start, size);
}

static inline __attribute__((always_inline)) void dcache_flush_all(void)
{
	arch_flush_dcache_all();
}

static inline __attribute__((always_inline)) void dcache_invalidate_all(void)
{
	arch_invalidate_dcache_all();
}

static inline __attribute__((always_inline)) uint32_t dcache_get_cacheline_size(void)
{
	return arch_dcache_get_cacheline_size();
}

static inline __attribute__((always_inline)) void data_sync_barrier(void)
{
	dsb();
}

#endif /* __ARM32_CACHE_H__ */
