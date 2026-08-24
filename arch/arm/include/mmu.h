/* SPDX-License-Identifier: GPL-2.0+ */

/** ARM32 short-descriptor MMU interface. */
#ifndef __ARM32_MMU_H__
#define __ARM32_MMU_H__

#include <stdint.h>

#include "barrier.h"
#include "cache.h"
#include "interrupt.h"
#include "timer.h"

struct arm_regs_t {
	uint32_t esp;
	uint32_t cpsr;
	uint32_t r[13];
	uint32_t sp;
	uint32_t lr;
	uint32_t pc;
};

/* SCTLR bits used by the ARM32 implementation. */
enum {
	ARM32_SCTLR_M = 1U << 0,
};

static inline uint32_t arm32_read_p15_c1(void)
{
	uint32_t value;

	__asm__ __volatile__("mrc p15, 0, %0, c1, c0, 0"
				     : "=r"(value) : : "memory");
	return value;
}

static inline void arm32_write_p15_c1(uint32_t value)
{
	__asm__ __volatile__("mcr p15, 0, %0, c1, c0, 0"
				     : : "r"(value) : "memory");
	/* A read-back prevents a following C access from passing the write. */
	(void)arm32_read_p15_c1();
}

/**
 * Build a 1MiB short-descriptor table in the top MiB of DRAM and enable the
 * MMU. dram_size is expressed in MiB, as returned by the DRAM driver.
 */
void arm32_mmu_enable(uint32_t dram_base, uint32_t dram_size);
void arm32_mmu_disable(void);

#endif /* __ARM32_MMU_H__ */
