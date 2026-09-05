/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __MMU_H__
#define __MMU_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "csr.h"
#include "timer.h"
#include "barrier.h"
#include "cache.h"
#include "interrupt.h"

/**
 * @file
 * @brief Memory Management Unit (MMU) interface for RISC-V architecture.
 *
 * This header file provides functions and definitions for initializing and 
 * managing the memory management unit (MMU) on RISC-V architecture.
 */

/**
 * @brief Enable the SV39 MMU with cache initialization.
 *
 * This function initializes the cache and enables both data and
 * instruction caches for the SV39 memory management unit.
 */
void mmu_enable(void);

/**
 * @brief Add an aligned memory region to the E907 system memory map.
 *
 * The region is appended after existing entries and receives the supplied
 * cacheability, shareability, and bufferability attributes. The implementation
 * rejects overlaps, unaligned limits, and a table that has reached capacity.
 *
 * @param[in] start_addr Inclusive region start, aligned to the map granule.
 * @param[in] len Region length in bytes; must be non-zero and aligned.
 * @param[in] mem_attr Encoded `SYSMAP_MEM_ATTR_*` flags.
 * @return Zero on success, or a `SYSMAP_RET_*` error code.
 */
int sysmap_add_mem_region(uint32_t start_addr, uint32_t len, uint32_t mem_attr);

#ifdef __cplusplus
}
#endif

#endif /* __MMU_H__ */
