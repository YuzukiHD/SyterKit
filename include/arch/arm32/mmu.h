/**
 * @file mmu.h
 * @brief Memory Management Unit (MMU) interface for ARM32 architecture.
 *
 * This header file provides functions and definitions for managing the Memory Management Unit
 * on ARM32 architecture. It includes functions for enabling/disabling MMU and MMU control operations.
 */
/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __MMU_H__
#define __MMU_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "timer.h"
#include "barrier.h"
#include "cache.h"
#include "interrupt.h"

/**
 * @brief ARM32 register structure.
 *
 * This structure defines the layout of ARM32 general purpose and special registers.
 * It is typically used for context switching or exception handling.
 */
struct arm_regs_t {
	uint32_t esp;	/**< Extended stack pointer */
	uint32_t cpsr;	/**< Current Program Status Register */
	uint32_t r[13]; /**< General purpose registers R0-R12 */
	uint32_t sp;	/**< Stack pointer (R13) */
	uint32_t lr;	/**< Link register (R14) */
	uint32_t pc;	/**< Program counter (R15) */
};

/**
 * @brief Read the ARM32 system control register (CP15, c1).
 *
 * This function reads the value of the ARM32 system control register (CP15, c1),
 * which controls various system features including MMU, caches, and alignment checking.
 *
 * @return The value of the system control register.
 */
static inline uint32_t arm32_read_p15_c1(void) {
	uint32_t value;

	__asm__ __volatile__("mrc p15, 0, %0, c1, c0, 0"
						 : "=r"(value)
						 :
						 : "memory");

	return value;
}

/**
 * @brief Write to the ARM32 system control register (CP15, c1).
 *
 * This function writes a value to the ARM32 system control register (CP15, c1),
 * which controls various system features including MMU, caches, and alignment checking.
 * A read-back is performed to ensure the write is completed.
 *
 * @param value The value to write to the system control register.
 */
static inline void arm32_write_p15_c1(uint32_t value) {
	__asm__ __volatile__("mcr p15, 0, %0, c1, c0, 0"
						 :
						 : "r"(value)
						 : "memory");
	arm32_read_p15_c1();
}

/**
 * @brief Enable the ARM32 MMU with specific memory configuration.
 *
 * This function initializes and enables the Memory Management Unit (MMU) for ARM32 architecture.
 * It sets up page tables with appropriate memory attributes and configures the MMU control registers.
 *
 * @param dram_base Base address of DRAM memory.
 * @param dram_size Size of DRAM memory in megabytes.
 *
 * @note The function creates a 1MB section-based page table in the highest 16MB of DRAM.
 *       It configures different memory regions with appropriate caching attributes.
 */
static inline void arm32_mmu_enable(const uint32_t dram_base, uint32_t dram_size) {
	uint32_t mmu_base;

	/* use dram high 16M */
	if (dram_size > 2048)
		dram_size = 2048;

	uint32_t *mmu_base_addr = (uint32_t *) (dram_base + ((dram_size - 1) << 20));
	uint32_t *page_table = mmu_base_addr;

	int i;
	uint32_t reg;

	/* the front 1M contain BROM/SRAM */
#ifdef CONFIG_CHIP_DCACHE
	page_table[0] = (3 << 10) | (15 << 5) | (1 << 3) | (1 << 2) | 0x2;
#else
	page_table[0] = (3 << 10) | (15 << 5) | (1 << 3) | (0 << 2) | 0x2;
#endif
	/* the front 1G of memory(treated as 4G for all) is set up as none cacheable */
	for (i = 1; i < (dram_base >> 20); i++) { page_table[i] = (i << 20) | (3 << 10) | (15 << 5) | (0 << 3) | 0x2; }
	/* Set up as write back and buffered for other 3GB, rw for everyone */
	for (i = (dram_base >> 20); i < 4096; i++) {
#ifdef CONFIG_CHIP_DCACHE
		page_table[i] = (i << 20) | (3 << 10) | (15 << 5) | (1 << 3) | (1 << 2) | 0x2;
#else
		page_table[i] = (i << 20) | (3 << 10) | (15 << 5) | (1 << 3) | (0 << 2) | 0x2;
#endif
	}
	/* flush tlb */
	asm volatile("mcr p15, 0, %0, c8, c7, 0"
				 :
				 : "r"(0));
	/* Copy the page table address to cp15 */
	mmu_base = (uint32_t) mmu_base_addr;
	mmu_base |= (1 << 0) | (1 << 1) | (2 << 3);
	asm volatile("mcr p15, 0, %0, c2, c0, 0"
				 :
				 : "r"(mmu_base)
				 : "memory");
	asm volatile("mcr p15, 0, %0, c2, c0, 1"
				 :
				 : "r"(mmu_base)
				 : "memory");
	/* Set the access control to all-supervisor */
	asm volatile("mcr p15, 0, %0, c3, c0, 0"
				 :
				 : "r"(0x55555555));//modified, origin value is (~0)
	asm volatile("isb");

#ifdef CONFIG_CHIP_DCACHE
	/* enable smp */
	asm volatile("mrc     p15, 0, r0, c1, c0, 1");
	asm volatile("orr     r0, r0, #0x040");
	asm volatile("mcr     p15, 0, r0, c1, c0, 1");
#endif

	/* and enable the mmu */
	asm volatile("mrc p15, 0, %0, c1, c0, 0	@ get CR"
				 : "=r"(reg)
				 :
				 : "cc");

	sdelay(100);
	reg |= ((1 << 0) | (1 << 12));// enable mmu, icache
	reg &= ~(1 << 2);			  // disable dcache

	printk_trace("MMU: CR = 0x%08x\n", reg);
	asm volatile("mcr p15, 0, %0, c1, c0, 0	@ set CR"
				 :
				 : "r"(reg)
				 : "cc");
	asm volatile("isb");
}

/**
 * @brief Disable the ARM32 MMU and clear caches.
 *
 * This function disables the Memory Management Unit (MMU), instruction cache,
 * and data cache. It also invalidates the instruction cache, branch predictor,
 * and ensures all operations are completed with memory barriers.
 */
static inline void arm32_mmu_disable(void) {
	uint32_t reg;

	/* and disable the mmu */
	asm volatile("mrc p15, 0, %0, c1, c0, 0	@ get CR"
				 : "=r"(reg)
				 :
				 : "cc");
	sdelay(100);
	reg &= ~((7 << 0) | (1 << 12));//disable mmu, icache, dcache
	asm volatile("mcr p15, 0, %0, c1, c0, 0	@ set CR"
				 :
				 : "r"(reg)
				 : "cc");
	asm volatile("isb");
	/*
	 * Invalidate all instruction caches to PoU.
	 * Also flushes branch target cache.
	 */
	asm volatile("mcr p15, 0, %0, c7, c5, 0"
				 :
				 : "r"(0));
	/* Invalidate entire branch predictor array */
	asm volatile("mcr p15, 0, %0, c7, c5, 6"
				 :
				 : "r"(0));
	/* Full system DSB - make sure that the invalidation is complete */
	asm volatile("dsb");
	/* ISB - make sure the instruction stream sees it */
	asm volatile("isb");
}

#ifdef __cplusplus
}
#endif

#endif /* __MMU_H__ */
