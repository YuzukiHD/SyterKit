/* SPDX-License-Identifier: Apache-2.0 */

#ifndef __MMU_H__
#define __MMU_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include <csr.h>
#include <timer.h>

#define L1_CACHE_BYTES (64)

static inline void data_sync_barrier(void) {
    asm volatile("fence.i");
}

static inline void cache_init(void) {
    csr_write(mcor, 0x70013);
    csr_write(mhcr, 0x11ff);
    csr_set(mxstatus, 0x638000);
    csr_write(mhint, 0x16e30c);
}

static inline void dcache_enable(void) {
    csr_write(mhcr, 0x2);
}

static inline void icache_enable(void) {
    csr_set(mhcr, 0x1);
}

static inline void sv39_mmu_enable(void) {
    cache_init();
    dcache_enable();
    icache_enable();
}

static inline void flush_dcache_range(uint64_t start, uint64_t end) {
    register uint64_t i asm("a0") = start & ~(L1_CACHE_BYTES - 1);
    for (; i < end; i += L1_CACHE_BYTES)
        asm volatile(".long 0x0295000b"); /* dcache.cpa a0 */
    asm volatile(".long 0x01b0000b");     /* sync.is */
}

static inline void invalidate_dcache_range(uint64_t start, uint64_t end) {
    register uint64_t i asm("a0") = start & ~(L1_CACHE_BYTES - 1);
    for (; i < end; i += L1_CACHE_BYTES)
        asm volatile("dcache.ipa a0");
    asm volatile(".long 0x01b0000b");
}

#ifdef __cplusplus
}
#endif

#endif /* __MMU_H__ */
