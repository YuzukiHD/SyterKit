/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file cache.c
 * @brief E907 system-memory-map and cache control implementation.
 *
 * E907 exposes a compact region table rather than a full page-table MMU.
 * Helpers below translate byte addresses to the hardware's shifted limits,
 * while the public cache routines provide the architecture hooks expected by
 * the common startup code.
 */

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

#include <e907/sysmap.h>
#include <string.h>

/* #define DEBUG_SYSMAP */
#define L1_CACHE_BYTES (32) /**< Size of L1 cache line in bytes. */

enum sysmap_ret_code {
	SYSMAP_RET_OK = 0,
	SYSMAP_RET_INVALID_MEM_ADDR = -100,
	SYSMAP_RET_INVALID_MEM_LEN,
	SYSMAP_RET_INVALID_MEM_ATTR,
	SYSMAP_RET_REGION_IS_FULL,
	SYSMAP_RET_REGION_NOT_ENOUGH,
};

static uint32_t region_index = 0;

/**
 * @brief Read one region's encoded upper address limit.
 * @param[in] region_index Region table index.
 * @return Encoded upper limit read from the hardware table.
 */
static inline uint32_t sysmap_region_get_upper_limit(uint32_t region_index)
{
	uint32_t reg_addr = PLAT_SYSMAP_BASE_ADDR + region_index * 8;

	return readl(reg_addr);
}

/**
 * @brief Read one region's encoded memory attributes.
 * @param[in] region_index Region table index.
 * @return Encoded attributes read from the hardware table.
 */
static inline uint32_t sysmap_region_get_mem_attr(uint32_t region_index)
{
	uint32_t reg_addr = PLAT_SYSMAP_BASE_ADDR + region_index * 8 + 4;

	return readl(reg_addr);
}

/**
 * @brief Write one region's encoded upper address limit.
 * @param[in] region_index Region table index.
 * @param[in] upper_limit_addr Encoded upper address value.
 */
static void sysmap_region_set_upper_limit(uint32_t region_index, uint32_t upper_limit_addr)
{
	uint32_t reg_addr = PLAT_SYSMAP_BASE_ADDR + region_index * 8;

	writel(upper_limit_addr, reg_addr);
}

/**
 * @brief Write one region's encoded memory attributes.
 * @param[in] region_index Region table index.
 * @param[in] mem_attr Encoded memory attributes.
 */
static void sysmap_region_set_mem_attr(uint32_t region_index, uint32_t mem_attr)
{
	uint32_t reg_addr = PLAT_SYSMAP_BASE_ADDR + region_index * 8 + 4;

	writel(mem_attr, reg_addr);
}

/**
 * @brief Convert a region limit register to a byte address.
 * @param[in] region_index Region table index.
 * @return Exclusive upper byte address.
 */
static inline uint32_t get_mem_region_upper_limit(uint32_t region_index)
{
	return sysmap_region_get_upper_limit(region_index) << SYSMAP_ADDR_SHIFT;
}

/**
 * @brief Return the first byte covered by a region.
 * @param[in] region_index Region table index.
 * @return Inclusive start byte address.
 */
static inline uint32_t get_mem_region_start_addr(uint32_t region_index)
{
	if (region_index == 0)
		return 0;
	else
		return get_mem_region_upper_limit(region_index - 1);
}

/**
 * @brief Return the last byte covered by a region.
 * @param[in] region_index Region table index.
 * @return Inclusive end byte address.
 */
static inline uint32_t get_mem_region_end_addr(uint32_t region_index)
{
	return get_mem_region_upper_limit(region_index) - 1;
}

/**
 * @brief Return a region length in bytes.
 * @param[in] region_index Region table index.
 * @return Number of bytes covered by the region.
 */
static inline uint32_t get_mem_region_len(uint32_t region_index)
{
	if (region_index == 0)
		return get_mem_region_upper_limit(region_index);
	else
		return get_mem_region_upper_limit(region_index) - get_mem_region_upper_limit(region_index - 1);
}

/**
 * @brief Return masked attributes for one configured region.
 * @param[in] region_index Region table index.
 * @return Masked hardware attributes.
 */
static inline uint32_t get_mem_region_attr(uint32_t region_index)
{
	return sysmap_region_get_mem_attr(region_index) & SYSMAP_MEM_ATTR_MASK;
}

/**
 * @brief Encode and write a region's byte upper limit.
 * @param[in] region_index Region table index.
 * @param[in] upper_limit_addr Exclusive upper byte address.
 */
static inline void set_mem_region_upper_limit(uint32_t region_index, uint32_t upper_limit_addr)
{
	sysmap_region_set_upper_limit(region_index, upper_limit_addr >> SYSMAP_ADDR_SHIFT);
}

/**
 * @brief Mask and write a region's memory attributes.
 * @param[in] region_index Region table index.
 * @param[in] mem_attr Memory attributes to apply.
 */
static inline void set_mem_region_attr(uint32_t region_index, uint32_t mem_attr)
{
	sysmap_region_set_mem_attr(region_index, mem_attr & SYSMAP_MEM_ATTR_MASK);
}

/**
 * @brief Program both limit and attributes for one region entry.
 * @param[in] region_index Region table index.
 * @param[in] upper_limit_addr Exclusive upper byte address.
 * @param[in] mem_attr Memory attributes to apply.
 */
static inline void sysmap_setup_mem_region(uint32_t region_index, uint32_t upper_limit_addr, uint32_t mem_attr)
{
	set_mem_region_attr(region_index, mem_attr);
	set_mem_region_upper_limit(region_index, upper_limit_addr);
}

/**
 * @brief Program a region from a start address and byte length.
 * @param[in] region_index Region table index.
 * @param[in] start_addr Inclusive start byte address.
 * @param[in] len Region length in bytes.
 * @param[in] mem_attr Memory attributes to apply.
 */
static inline void setup_mem_region(uint32_t region_index, uint32_t start_addr, uint32_t len, uint32_t mem_attr)
{
	sysmap_setup_mem_region(region_index, start_addr + len, mem_attr);
}

/*
 * @brief Append an aligned memory region to the E907 system map.
 * @param[in] start_addr Region start address, aligned to the hardware granule.
 * @param[in] len Region length in bytes.
 * @param[in] mem_attr Masked system-map attributes.
 * @return A `SYSMAP_RET_*` status describing validation or capacity failure.
 */
int sysmap_add_mem_region(uint32_t start_addr, uint32_t len, uint32_t mem_attr)
{
	uint32_t current_region_start_addr;

	if (!IS_MEM_ADDR_ALIGNED(start_addr))
		return SYSMAP_RET_INVALID_MEM_ADDR;

	if (!len)
		return SYSMAP_RET_INVALID_MEM_LEN;

	if ((mem_attr & SYSMAP_MEM_ATTR_MASK) != mem_attr)
		return SYSMAP_RET_INVALID_MEM_ATTR;

	if (region_index >= SYSMAP_REGION_NUM)
		return SYSMAP_RET_REGION_IS_FULL;

	current_region_start_addr = 0;
	if (region_index)
		current_region_start_addr = get_mem_region_start_addr(region_index);

	if (start_addr < current_region_start_addr)
		return SYSMAP_RET_INVALID_MEM_ADDR;

	if (start_addr > current_region_start_addr) {
		if (region_index == (SYSMAP_REGION_NUM - 1))
			return SYSMAP_RET_REGION_NOT_ENOUGH;

		sysmap_setup_mem_region(region_index, start_addr, SYSMAP_MEM_ATTR_SO_NC_NB);
		region_index++;
	}

	setup_mem_region(region_index, start_addr, len, mem_attr);
	region_index++;
	return SYSMAP_RET_OK;
}

/**
 * @brief Print all configured E907 system-map regions in debug builds.
 *
 * With `DEBUG_SYSMAP` disabled this is a no-op, retaining the public hook
 * without adding release-image logging overhead.
 */
void sysmap_dump_region_info(void)
{
#ifdef DEBUG_SYSMAP
	uint32_t i, mem_attr;
	char mem_attr_so_str[4] = { 0 }; // Buffer for "SO" or "WO"
	char mem_attr_cache_str[6] = { 0 }; // Buffer for "_C_" or "_NC_"
	char mem_attr_buff_str[3] = { 0 }; // Buffer for "B" or "NB"
	uint32_t len;

	pr_debug("E907 SYSMAP INFO:\n");
	for (i = 0; i < SYSMAP_REGION_NUM; i++) {
		mem_attr = get_mem_region_attr(i);

		if (mem_attr & SYSMAP_MEM_ATTR_SO) {
			memcpy(mem_attr_so_str, "SO", 2);
		} else {
			memcpy(mem_attr_so_str, "WO", 2);
		}

		if (mem_attr & SYSMAP_MEM_ATTR_CACHEABLE) {
			memcpy(mem_attr_cache_str, "_C_", 3);
		} else {
			memcpy(mem_attr_cache_str, "_NC_", 4);
		}

		if (mem_attr & SYSMAP_MEM_ATTR_BUFFERABLE) {
			memcpy(mem_attr_buff_str, "B", 1);
		} else {
			memcpy(mem_attr_buff_str, "NB", 2);
		}

		pr_debug("Region %u, start: 0x%08x, end: 0x%08x, len: 0x%08x, attr: %s%s%s (0x%x)\n", i, get_mem_region_start_addr(i), get_mem_region_end_addr(i),
			     get_mem_region_len(i), mem_attr_so_str, mem_attr_cache_str, mem_attr_buff_str, mem_attr);
	}
#endif
	return;
}

/**
 * @brief Insert a data synchronization barrier.
 *
 * This function ensures that all previous instructions are completed
 * before any subsequent instructions are executed, particularly useful 
 * for ensuring memory consistency.
 */
void data_sync_barrier(void)
{
	asm volatile("fence.i");
}

/**
 * @brief Initialize E907 cache control and prefetch settings.
 *
 * This routine only programs control CSRs; it does not enable either cache
 * until the corresponding explicit enable hook is called.
 */
void cache_init(void)
{ // Configure cache options
	csr_write(mhcr, MHCR_WB | MHCR_WA | MHCR_RS | MHCR_BPE | MHCR_BTE); // Set cache hit control register
	csr_write(mhint, MHINT_D_PLD | MHINT_IWPE | MHINT_AMR_1 | MHINT_PREF_N_16); // Set hint for cache operations
}

/**
 * @brief Enable the data cache.
 *
 * This function enables the data cache by writing to the machine 
 * cache control register.
 */
void dcache_enable(void)
{
	csr_set(mhcr, MHCR_DE); // Set the data cache enable bit
}

/**
 * @brief Enable the instruction cache.
 *
 * This function enables the instruction cache by setting the 
 * appropriate control bits in the machine cache control register.
 */
void icache_enable(void)
{
	csr_set(mhcr, MHCR_IE); // Set the instruction cache enable bit
}

/**
 * @brief Enable the SV39 MMU with cache initialization.
 *
 * This function initializes the cache and enables both data and
 * instruction caches for the SV39 memory management unit.
 */
void mmu_enable(void)
{
	return;
}

void flush_dcache_range(uint64_t start, uint64_t end)
{
	register uint32_t i asm("a0") = start & ~(L1_CACHE_BYTES - 1);
	for (; i < end; i += L1_CACHE_BYTES)
		asm volatile("dcache.cpa a0");
	asm volatile("sync.i");
}

void invalidate_dcache_range(uint64_t start, uint64_t end)
{
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
void flush_dcache_all()
{
	asm volatile("dcache.call");
}

/**
 * @brief Invalidates the entire data cache.
 *
 * This function invalidates all data cache lines, ensuring that no stale or outdated
 * data remains in the cache. This operation discards the cache contents and ensures that
 * the next access will fetch fresh data from memory.
 */
void invalidate_dcache_all()
{
	asm volatile("dcache.ciall");
}
