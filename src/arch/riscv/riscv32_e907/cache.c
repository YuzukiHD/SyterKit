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
#define PLAT_SYSMAP_BASE_ADDR (SUNXI_PLAT_TCIP_BASE_ADDR + 0xFFFF000)

#define SYSMAP_REGION_NUM 8

#define SYSMAP_MEM_ATTR_SO 0x10
#define SYSMAP_MEM_ATTR_CACHEABLE 0x8
#define SYSMAP_MEM_ATTR_BUFFERABLE 0x4

#define SYSMAP_MEM_ATTR_MASK (SYSMAP_MEM_ATTR_SO | SYSMAP_MEM_ATTR_CACHEABLE | SYSMAP_MEM_ATTR_BUFFERABLE)

#define SYSMAP_MEM_ATTR_SO_NC_NB (SYSMAP_MEM_ATTR_SO)
#define SYSMAP_MEM_ATTR_SO_NC_B (SYSMAP_MEM_ATTR_SO | SYSMAP_MEM_ATTR_BUFFERABLE)

#define SYSMAP_MEM_ATTR_WO_NC_NB (0)
#define SYSMAP_MEM_ATTR_WO_NC_B (SYSMAP_MEM_ATTR_BUFFERABLE)
#define SYSMAP_MEM_ATTR_WO_C_NB (SYSMAP_MEM_ATTR_CACHEABLE)
#define SYSMAP_MEM_ATTR_WO_C_B (SYSMAP_MEM_ATTR_CACHEABLE | SYSMAP_MEM_ATTR_BUFFERABLE)

#define SYSMAP_MEM_ATTR_DEVICE (SYSMAP_MEM_ATTR_SO_NC_NB)
#define SYSMAP_MEM_ATTR_RAM (SYSMAP_MEM_ATTR_WO_C_B)

#define SYSMAP_ADDR_SHIFT 12
#define SYSMAP_ADDR_ALIGN_SIZE (1 << SYSMAP_ADDR_SHIFT)
#define IS_MEM_ADDR_ALIGNED(addr) (!(addr & (SYSMAP_ADDR_ALIGN_SIZE - 1)))

enum sysmap_ret_code {
    SYSMAP_RET_OK = 0,
    SYSMAP_RET_INVALID_MEM_ADDR = -100,
    SYSMAP_RET_INVALID_MEM_LEN,
    SYSMAP_RET_INVALID_MEM_ATTR,
    SYSMAP_RET_REGION_IS_FULL,
    SYSMAP_RET_REGION_NOT_ENOUGH,
};

static uint32_t region_index = 0;

static inline uint32_t sysmap_region_get_upper_limit(uint32_t region_index) {
    uint32_t reg_addr = PLAT_SYSMAP_BASE_ADDR + region_index * 8;

    return readl(reg_addr);
}

static inline uint32_t sysmap_region_get_mem_attr(uint32_t region_index) {
    uint32_t reg_addr = PLAT_SYSMAP_BASE_ADDR + region_index * 8 + 4;

    return readl(reg_addr);
}

static void sysmap_region_set_upper_limit(uint32_t region_index, uint32_t upper_limit_addr) {
    uint32_t reg_addr = PLAT_SYSMAP_BASE_ADDR + region_index * 8;

    writel(upper_limit_addr, reg_addr);
}

static void sysmap_region_set_mem_attr(uint32_t region_index, uint32_t mem_attr) {
    uint32_t reg_addr = PLAT_SYSMAP_BASE_ADDR + region_index * 8 + 4;

    writel(mem_attr, reg_addr);
}

static inline uint32_t get_mem_region_upper_limit(uint32_t region_index) {
    return sysmap_region_get_upper_limit(region_index) << SYSMAP_ADDR_SHIFT;
}

static inline uint32_t get_mem_region_start_addr(uint32_t region_index) {
    if (region_index == 0)
        return 0;
    else
        return get_mem_region_upper_limit(region_index - 1);
}

static inline uint32_t get_mem_region_end_addr(uint32_t region_index) {
    return get_mem_region_upper_limit(region_index) - 1;
}

static inline uint32_t get_mem_region_len(uint32_t region_index) {
    if (region_index == 0)
        return get_mem_region_upper_limit(region_index);
    else
        return get_mem_region_upper_limit(region_index) - get_mem_region_upper_limit(region_index - 1);
}

static inline uint32_t get_mem_region_attr(uint32_t region_index) {
    return sysmap_region_get_mem_attr(region_index) & SYSMAP_MEM_ATTR_MASK;
}

static inline void set_mem_region_upper_limit(uint32_t region_index, uint32_t upper_limit_addr) {
    sysmap_region_set_upper_limit(region_index, upper_limit_addr >> SYSMAP_ADDR_SHIFT);
}

static inline void set_mem_region_attr(uint32_t region_index, uint32_t mem_attr) {
    sysmap_region_set_mem_attr(region_index, mem_attr & SYSMAP_MEM_ATTR_MASK);
}

static inline void sysmap_setup_mem_region(uint32_t region_index, uint32_t upper_limit_addr, uint32_t mem_attr) {
    set_mem_region_attr(region_index, mem_attr);
    set_mem_region_upper_limit(region_index, upper_limit_addr);
}

static inline void setup_mem_region(uint32_t region_index,
                                    uint32_t start_addr, uint32_t len, uint32_t mem_attr) {
    sysmap_setup_mem_region(region_index, start_addr + len, mem_attr);
}

int sysmap_add_mem_region(uint32_t start_addr, uint32_t len, uint32_t mem_attr) {
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

void init_sysmap() {
    sysmap_add_mem_region(0x00000000, 0x10000000, SYSMAP_MEM_ATTR_RAM);
    sysmap_add_mem_region(0x10000000, 0x02000000, SYSMAP_MEM_ATTR_RAM);
    sysmap_add_mem_region(0x12000000, 0x1E000000, SYSMAP_MEM_ATTR_DEVICE);
    sysmap_add_mem_region(0x30000000, 0x10000000, SYSMAP_MEM_ATTR_DEVICE);
    sysmap_add_mem_region(0x40000000, 0x28000000, SYSMAP_MEM_ATTR_DEVICE);
    sysmap_add_mem_region(0x68000000, 0x01000000, SYSMAP_MEM_ATTR_DEVICE);
    sysmap_add_mem_region(0x69000000, 0x17000000, SYSMAP_MEM_ATTR_DEVICE);
    sysmap_add_mem_region(0x80000000, 0x7FFFFFFF, SYSMAP_MEM_ATTR_RAM);
}

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
void cache_init(void) {
    init_sysmap();                                                               // Configure cache options
    csr_write(mhcr, MHCR_IE | MHCR_DE | MHCR_WB | MHCR_WA | MHCR_RS | MHCR_BPE | MHCR_BTE);// Set cache hit control register                                              // Set machine status register
    csr_write(mhint, MHINT_D_PLD | MHINT_IWPE | MHINT_AMR_1 | MHINT_PREF_N_16);  // Set hint for cache operations
}

/**
 * @brief Enable the data cache.
 *
 * This function enables the data cache by writing to the machine 
 * cache control register.
 */
void dcache_enable(void) {
    csr_write(mhcr, 0x2);// Set the data cache enable bit
}

/**
 * @brief Enable the instruction cache.
 *
 * This function enables the instruction cache by setting the 
 * appropriate control bits in the machine cache control register.
 */
void icache_enable(void) {
    csr_set(mhcr, 0x1);// Set the instruction cache enable bit
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
