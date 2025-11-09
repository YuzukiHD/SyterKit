/**
 * @file barrier.h
 * @brief Memory barrier definitions for RISC-V architecture.
 *
 * This header file provides architecture-specific memory barrier macros
 * for RISC-V processors. Memory barriers are essential for ensuring
 * proper memory ordering between different CPUs or between CPUs and devices.
 *
 * The implementation uses the RISC-V fence instruction with different
 * parameters to provide various types of memory barriers.
 */
/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __RISCV_BARRIER_H__
#define __RISCV_BARRIER_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Generic RISC-V fence instruction macro.
 *
 * This macro generates a RISC-V fence instruction with the specified
 * predecessor and successor memory access types. The fence instruction
 * ensures that all memory accesses of types 'p' that appear before the fence
 * in program order are visible before any memory accesses of types 's'
 * that appear after the fence.
 *
 * @param p Memory access types for predecessor operations (before the fence).
 *          Can be combinations of:
 *          - 'i': instruction loads
 *          - 'o': memory loads
 *          - 'r': memory loads
 *          - 'w': memory stores
 * @param s Memory access types for successor operations (after the fence).
 *          Can be combinations of the same values as 'p'.
 *
 * @note The fence instruction also acts as a compiler barrier via the
 *       "memory" clobber specification.
 */
#define RISCV_FENCE(p, s) asm volatile("fence " #p "," #s \
									   :                  \
									   :                  \
									   : "memory")

/**
 * @brief Full memory barrier.
 *
 * Ensures that all memory accesses (loads and stores) from or to memory
 * that appear before the barrier in program order are completed before any
 * memory accesses (loads and stores) that appear after the barrier.
 * This includes both instruction and data memory accesses.
 */
#define mb() RISCV_FENCE(iorw, iorw)

/**
 * @brief Read memory barrier.
 *
 * Ensures that all memory reads (both instruction and data) that appear
 * before the barrier in program order are completed before any memory
 * reads that appear after the barrier.
 */
#define rmb() RISCV_FENCE(ir, ir)

/**
 * @brief Write memory barrier.
 *
 * Ensures that all memory writes that appear before the barrier in
 * program order are completed before any memory writes that appear
 * after the barrier.
 */
#define wmb() RISCV_FENCE(ow, ow)

/**
 * @brief SMP memory barrier.
 *
 * Ensures that all memory accesses (loads and stores) that appear before
 * the barrier in program order are completed before any memory accesses
 * (loads and stores) that appear after the barrier, but only with respect
 * to other CPUs (not devices).
 */
#define __smp_mb() RISCV_FENCE(rw, rw)

/**
 * @brief SMP read memory barrier.
 *
 * Ensures that all memory reads that appear before the barrier in
 * program order are completed before any memory reads that appear
 * after the barrier, but only with respect to other CPUs.
 */
#define __smp_rmb() RISCV_FENCE(r, r)

/**
 * @brief SMP write memory barrier.
 *
 * Ensures that all memory writes that appear before the barrier in
 * program order are completed before any memory writes that appear
 * after the barrier, but only with respect to other CPUs.
 */
#define __smp_wmb() RISCV_FENCE(w, w)

#ifdef __cplusplus
}
#endif

#endif /* __RISCV_BARRIER_H__ */