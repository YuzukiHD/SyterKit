/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file mmu.h
 * @brief ARM32 short-descriptor MMU interface and saved-register layout.
 */
#ifndef __ARM32_MMU_H__
#define __ARM32_MMU_H__

#include <stdint.h>

#include "barrier.h"
#include "cache.h"
#include "interrupt.h"
#include "timer.h"

struct arm_regs_t {
	/** Saved exception stack pointer. */
	uint32_t esp;
	/** Saved CPSR value, including the processor mode bits. */
	uint32_t cpsr;
	/** Saved general-purpose registers r0 through r12. */
	uint32_t r[13];
	/** Saved stack pointer. */
	uint32_t sp;
	/** Saved link register. */
	uint32_t lr;
	/** Saved program counter. */
	uint32_t pc;
};

/* SCTLR bits used by the ARM32 implementation. */
enum {
	ARM32_SCTLR_M = 1U << 0,
};

/**
 * @brief Read the ARM system-control register (SCTLR).
 * @return Current CP15 c1 control value.
 */
static inline uint32_t arm32_read_p15_c1(void)
{
	uint32_t value;

	__asm__ __volatile__("mrc p15, 0, %0, c1, c0, 0" : "=r"(value) : : "memory");
	return value;
}

/**
 * @brief Write the ARM system-control register and force a read-back.
 * @param[in] value New SCTLR value.
 */
static inline void arm32_write_p15_c1(uint32_t value)
{
	__asm__ __volatile__("mcr p15, 0, %0, c1, c0, 0" : : "r"(value) : "memory");
	/* A read-back prevents a following C access from passing the write. */
	(void)arm32_read_p15_c1();
}

/**
 * @brief Build a section table and enable the ARM short-descriptor MMU.
 *
 * A section entry is emitted for each MiB of the supplied DRAM span. The
 * table is placed in the final MiB of DRAM, so callers must reserve that area
 * and pass the size reported by the DRAM driver.
 *
 * Invalid or empty DRAM windows are rejected without changing the active
 * translation regime. Sizes beyond the section-table limit are clamped.
 *
 * @param[in] dram_base Physical base address of DRAM.
 * @param[in] dram_size DRAM size in MiB.
 */
void arm32_mmu_enable(uint32_t dram_base, uint32_t dram_size);

/**
 * @brief Disable the ARM MMU and invalidate translation state.
 */
void arm32_mmu_disable(void);

#endif /* __ARM32_MMU_H__ */
