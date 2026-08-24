/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdint.h>

#include "barrier.h"
#include "log.h"
#include "mmu.h"

#define MMU_SECTION_SHIFT 20U
#define MMU_SECTION_COUNT 4096U
#define MMU_TABLE_BYTES (MMU_SECTION_COUNT * sizeof(uint32_t))
#define MMU_TABLE_ALIGN 0x4000U
#define ARM32_DRAM_SIZE_LIMIT 2048U

/* Short-descriptor level-1 section attributes. */
#define TTB_SECT_SECURE (0U << 19)
#define TTB_SECT_AP (3U << 10) /* privileged/user read-write */
#define TTB_SECT_DOMAIN(x) (((x) & 0xfU) << 5)
#define TTB_SECT_C (1U << 3)
#define TTB_SECT_B (1U << 2)
#define TTB_SECT (2U << 0)
#define TTB_SECT_BASE(x) ((x) << MMU_SECTION_SHIFT)

#define MMU_DOMAIN 15U
#define DACR_ALL_CLIENT 0x55555555U

/* TTBR0 inner/outer write-back, shareable, write-allocate attributes. */
#define TTBR0_IRGN_WB_WA (1U << 0)
#define TTBR0_S (1U << 1)
#define TTBR0_RGN_WT (2U << 3)

static void invalidate_tlb(void)
{
	uint32_t zero = 0;

	/* Invalidate unified, data and instruction TLBs. */
	__asm__ __volatile__("mcr p15, 0, %0, c8, c7, 0" : : "r"(zero) : "memory");
	__asm__ __volatile__("mcr p15, 0, %0, c8, c6, 0" : : "r"(zero) : "memory");
	__asm__ __volatile__("mcr p15, 0, %0, c8, c5, 0" : : "r"(zero) : "memory");
	dsb();
	isb();
}

static void invalidate_icache(void)
{
	uint32_t zero = 0;

	/* Invalidate instruction cache and branch predictor before changing M. */
	__asm__ __volatile__("mcr p15, 0, %0, c7, c5, 0" : : "r"(zero) : "memory");
	__asm__ __volatile__("mcr p15, 0, %0, c7, c5, 6" : : "r"(zero) : "memory");
	dsb();
	isb();
}

static uint32_t section_desc(uint32_t section, uint32_t cacheable)
{
	uint32_t desc = TTB_SECT_BASE(section) | TTB_SECT_AP | TTB_SECT_SECURE | TTB_SECT_DOMAIN(MMU_DOMAIN) | TTB_SECT;

	if (cacheable != 0U) {
		desc |= TTB_SECT_C;
#ifdef CONFIG_ARCH_DCACHE
		desc |= TTB_SECT_B;
#endif
	}
	return desc;
}

static void init_mapping(uint32_t *page_table, uint32_t dram_base, uint32_t dram_size)
{
	uint32_t base_section = dram_base >> MMU_SECTION_SHIFT;
	uint32_t index;

	/* Start from a complete, non-cacheable identity map for device MMIO. */
	for (index = 0; index < MMU_SECTION_COUNT; index++)
		page_table[index] = section_desc(index, 0U);

	/* SRAM/BROM and the detected DRAM are normal memory. */
	page_table[0] = section_desc(0U, 1U);
	for (index = base_section; index < base_section + dram_size; index++)
		page_table[index] = section_desc(index, 1U);
}

static void set_ttbr(uint32_t page_table)
{
	uint32_t ttbr0 = page_table | TTBR0_IRGN_WB_WA | TTBR0_S | TTBR0_RGN_WT;
	uint32_t zero = 0;

	__asm__ __volatile__("mcr p15, 0, %0, c2, c0, 0" : : "r"(ttbr0) : "memory");
	__asm__ __volatile__("mcr p15, 0, %0, c2, c0, 1" : : "r"(zero) : "memory");
}

static void set_dacr(void)
{
	uint32_t dacr = DACR_ALL_CLIENT;

	__asm__ __volatile__("mcr p15, 0, %0, c3, c0, 0" : : "r"(dacr) : "memory");
	isb();
}

static void enable_smp(void)
{
#ifdef CONFIG_ARCH_DCACHE
	uint32_t actlr;

	/* ACTLR.SMP is required for coherent shared data cache operation. */
	__asm__ __volatile__("mrc p15, 0, %0, c1, c0, 1" : "=r"(actlr));
	actlr |= (1U << 6);
	__asm__ __volatile__("mcr p15, 0, %0, c1, c0, 1" : : "r"(actlr) : "memory");
	isb();
#endif
}

void arm32_mmu_enable(uint32_t dram_base, uint32_t dram_size)
{
	uint64_t table_address;
	uint32_t base_section;
	uint32_t *page_table;
	uint32_t reg;

	/* A zero-sized or non-32-bit DRAM window cannot hold a page table. */
	if (dram_size == 0U)
		return;
	if (dram_size > ARM32_DRAM_SIZE_LIMIT)
		dram_size = ARM32_DRAM_SIZE_LIMIT;
	base_section = dram_base >> MMU_SECTION_SHIFT;
	if (base_section >= MMU_SECTION_COUNT || dram_size > MMU_SECTION_COUNT - base_section)
		dram_size = MMU_SECTION_COUNT - base_section;
	if (dram_size == 0U)
		return;

	/* Place the 16KiB table in the final MiB of the usable DRAM window. */
	table_address = (uint64_t)dram_base + ((uint64_t)(dram_size - 1U) << MMU_SECTION_SHIFT);
	table_address &= ~(uint64_t)(MMU_TABLE_ALIGN - 1U);
	if (table_address > 0xffffffffULL - MMU_TABLE_BYTES)
		return;
	page_table = (uint32_t *)(uintptr_t)table_address;

	reg = arm32_read_p15_c1();
	if ((reg & ARM32_SCTLR_C) != 0U)
		flush_dcache_range(table_address, table_address + MMU_TABLE_BYTES);

	init_mapping(page_table, dram_base, dram_size);
	/* Page-table writes must reach PoC before TTBR0 is installed. */
	flush_dcache_range(table_address, table_address + MMU_TABLE_BYTES);

	invalidate_tlb();
	set_ttbr((uint32_t)table_address);
	set_dacr();
	enable_smp();
	invalidate_icache();

	reg = arm32_read_p15_c1();
	reg |= ARM32_SCTLR_M;
	/* Enable both caches only after the MMU and its memory attributes are live. */
	reg &= ~(ARM32_SCTLR_C | ARM32_SCTLR_I);
	udelay(100);
	arm32_write_p15_c1(reg);
	dsb();
	isb();

#ifdef CONFIG_ARCH_ICACHE
	arm32_icache_enable();
#endif

#ifdef CONFIG_ARCH_DCACHE
	arm32_dcache_enable();
#endif

	reg = arm32_read_p15_c1();
	printk_trace("MMU: table=0x%08x dram=0x%08x size=%uMiB CR=0x%08x\n", (uint32_t)table_address, dram_base, dram_size, reg);
}

void arm32_mmu_disable(void)
{
	uint32_t reg = arm32_read_p15_c1();

	/* Disable each configured cache while the old translation regime is active. */
#ifdef CONFIG_ARCH_DCACHE
	if ((reg & ARM32_SCTLR_C) != 0U)
		arm32_dcache_disable();
#endif
#ifdef CONFIG_ARCH_ICACHE
	if ((reg & ARM32_SCTLR_I) != 0U)
		arm32_icache_disable();
#endif

	reg = arm32_read_p15_c1();
	reg &= ~ARM32_SCTLR_M;
	udelay(100);
	arm32_write_p15_c1(reg);
	dsb();
	invalidate_tlb();
}
