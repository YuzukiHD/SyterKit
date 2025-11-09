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
 * @file mmu.h
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
 * @brief Adds a memory region to the system memory map.
 *
 * This function registers a new memory region with the given starting address,
 * length, and memory attributes to the system's memory map. The region can be
 * used for various purposes, such as memory allocation or defining regions for
 * specific hardware access.
 *
 * @param start_addr Starting address of the memory region.
 *                   This address should be aligned to the appropriate boundary
 *                   for the memory type.
 *
 * @param len Length of the memory region in bytes.
 *            The length should be a positive value and should not exceed the
 *            system's available memory range.
 *
 * @param mem_attr Memory attributes for the region (e.g., read/write, cacheable,
 *                 non-cacheable, etc.). The exact attributes depend on the
 *                 platform and memory type.
 *
 * @return 0 if the memory region was successfully added.
 *         A non-zero value if the addition failed (e.g., invalid address or
 *         conflicting region).
 *
 * @note This function assumes that the provided address and length are valid.
 *       Any conflicts with existing memory regions will result in failure.
 */
int sysmap_add_mem_region(uint32_t start_addr, uint32_t len, uint32_t mem_attr);

#ifdef __cplusplus
}
#endif

#endif /* __MMU_H__ */
