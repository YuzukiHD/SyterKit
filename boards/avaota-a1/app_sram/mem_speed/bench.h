/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __MEM_SPEED_BENCH_H__
#define __MEM_SPEED_BENCH_H__

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Run the DRAM throughput and latency benchmarks.
 *
 * Measures sequential memcpy / memset / read / write throughput across block
 * sizes that step from the L1 cache up to the DRAM capacity, followed by a
 * pointer-chase access-latency pass. Results are printed over the debug UART.
 *
 * @param[in] base CPU-visible base address of the DRAM window.
 * @param[in] dram_bytes Total size of the DRAM window in bytes.
 */
void mem_speed_bench(uintptr_t base, size_t dram_bytes);

#endif /* __MEM_SPEED_BENCH_H__ */
