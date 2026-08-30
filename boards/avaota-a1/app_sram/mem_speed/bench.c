/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * DRAM performance benchmark for SyterKit.
 *
 * Reports sequential memcpy / memset / read / write throughput across block
 * sizes that step from the L1 cache up to the DRAM capacity, so the results
 * expose where the memory hierarchy stops being cache and starts being DRAM.
 * A pointer-chase pass reports access latency in nanoseconds.
 *
 * Every timed pass is preceded by a data-cache invalidate over its working
 * range, so large-block numbers reflect real DRAM streaming rather than a
 * cache left warm by the previous test.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <types.h>

#include <log.h>
#include <string.h>
#include <timer.h>

#include <common.h>
#include <cache.h>

#include "bench.h"

/* Keeps the outcome of a timed pass observable so -Os cannot drop the loop. */
static volatile uint32_t bench_sink;

/* Convert a byte count and elapsed microseconds into MiB/s. */
static uint32_t bytes_per_mib_s(size_t bytes, uint64_t us)
{
	if (us == 0)
		return 0;

	return (uint32_t)(((uint64_t)bytes * 1000000ULL) / us / (1024U * 1024U));
}

/* Drop cached lines covering the working range so the pass starts cold. */
static void bench_invalidate(uintptr_t addr, size_t bytes)
{
	invalidate_dcache_range(addr, addr + bytes);
}

/*
 * Sequential 32-bit read pass.
 *
 * Four independent accumulators break the add dependency chain that a single
 * "acc += buf[i]" loop would otherwise serialize on, so up to four loads are
 * in flight and the loop measures DRAM throughput rather than the LSU latency
 * of one dependent add. Block sizes are multiples of 4 KiB, so count is always
 * divisible by four.
 */
static uint64_t bench_read(uintptr_t addr, size_t bytes)
{
	volatile uint32_t *buf = (volatile uint32_t *)addr;
	size_t count = bytes / sizeof(uint32_t);
	uint32_t a0 = 0, a1 = 0, a2 = 0, a3 = 0;
	uint64_t t0, t1;

	bench_invalidate(addr, bytes);

	t0 = time_us();
	for (size_t i = 0; i < count; i += 4) {
		a0 += buf[i];
		a1 += buf[i + 1];
		a2 += buf[i + 2];
		a3 += buf[i + 3];
	}
	t1 = time_us();

	bench_sink = a0 + a1 + a2 + a3;
	return t1 - t0;
}

/*
 * Sequential 32-bit write pass.
 *
 * Four independent constant stores per iteration, mirroring what memset()
 * does with STM, so the store pipeline rather than the loop overhead is what
 * limits the result.
 */
static uint64_t bench_write(uintptr_t addr, size_t bytes)
{
	volatile uint32_t *buf = (volatile uint32_t *)addr;
	size_t count = bytes / sizeof(uint32_t);
	uint64_t t0, t1;

	bench_invalidate(addr, bytes);

	t0 = time_us();
	for (size_t i = 0; i < count; i += 4) {
		buf[i] = 0xa5a5a5a5;
		buf[i + 1] = 0xa5a5a5a5;
		buf[i + 2] = 0xa5a5a5a5;
		buf[i + 3] = 0xa5a5a5a5;
	}
	t1 = time_us();

	bench_sink = buf[0];
	return t1 - t0;
}

/* Bulk copy pass using the project's assembly-optimised memcpy(). */
static uint64_t bench_memcpy(uintptr_t dst, uintptr_t src, size_t bytes)
{
	uint64_t t0, t1;

	bench_invalidate(src, bytes);
	bench_invalidate(dst, bytes);

	t0 = time_us();
	memcpy((void *)dst, (void *)src, bytes);
	t1 = time_us();

	bench_sink = *(volatile uint32_t *)dst;
	return t1 - t0;
}

/* Bulk fill pass using the project's memset(). */
static uint64_t bench_memset(uintptr_t addr, size_t bytes)
{
	uint64_t t0, t1;

	bench_invalidate(addr, bytes);

	t0 = time_us();
	memset((void *)addr, 0x5a, bytes);
	t1 = time_us();

	bench_sink = *(volatile uint32_t *)addr;
	return t1 - t0;
}

/*
 * Pointer-chase access latency.
 *
 * A ring of pointer slots is laid out STRIDE bytes (one cache line) apart and
 * chained in a full-cycle permutation so hardware prefetchers cannot mask the
 * miss penalty. Walking the ring ITERATIONS times and dividing the elapsed
 * time by the hop count yields the effective access latency. A ring larger
 * than the cache thrashes it, so the figure reflects DRAM latency rather than
 * a resident L2 set.
 */
static void bench_latency(uintptr_t base, size_t avail)
{
	enum {
		STRIDE = 64,
		ITERATIONS = 200000,
		MAX_RING = 32 * 1024 * 1024,
	};
	size_t ring_bytes = avail < MAX_RING ? avail : MAX_RING;
	size_t max_entries = ring_bytes / STRIDE;
	size_t entries = 1;
	size_t gap;
	uint64_t t0, t1;
	uint32_t ns;

	/* Round the entry count down to a power of two; with 2^k entries any
	 * odd hop gap visits every slot exactly once before closing the cycle. */
	while ((entries << 1) <= max_entries)
		entries <<= 1;
	if (entries < 64)
		return;

	gap = entries / 2 + 1;
	bench_invalidate(base, entries * STRIDE);
	for (size_t cur = 0; cur < entries; cur++) {
		size_t next = (cur + gap) % entries;
		*(volatile uintptr_t *)(base + cur * STRIDE) = base + next * STRIDE;
	}

	volatile uintptr_t *p = (volatile uintptr_t *)base;
	t0 = time_us();
	for (uint32_t i = 0; i < ITERATIONS; i++)
		p = (volatile uintptr_t *)*p;
	t1 = time_us();

	bench_sink = (uint32_t)(uintptr_t)p;

	ns = (uint32_t)(((t1 - t0) * 1000ULL) / ITERATIONS);
	pr_info("Latency (%uKB ring, %uB stride): %u ns/access\n",
		(uint32_t)((entries * STRIDE) / 1024), STRIDE, ns);
}

/**
 * @brief Run the DRAM throughput and latency benchmarks.
 */
void mem_speed_bench(uintptr_t base, size_t dram_bytes)
{
	/* The MMU reserves the final MiB of DRAM for its section table; keep
	 * the working set clear of it. memcpy uses 2x the block size. */
	size_t avail = dram_bytes > (2 * 1024 * 1024) ? dram_bytes - (2 * 1024 * 1024) : 0;

	static const uint32_t sizes[] = {
		4 * 1024, 16 * 1024, 64 * 1024, 256 * 1024,
		1024 * 1024, 4 * 1024 * 1024, 16 * 1024 * 1024,
		32 * 1024 * 1024, 64 * 1024 * 1024,
	};

	pr_info("\n=========== DRAM Performance Test ===========\n");
	pr_info(" Base: 0x%08x, Size: %u MB\n",
		(uint32_t)base, (uint32_t)(dram_bytes / (1024U * 1024U)));
	pr_info(" block      memcpy     memset      read     write\n");
	pr_info("--------   -------   -------   -------   -------\n");

	for (size_t i = 0; i < ARRAY_SIZE(sizes); i++) {
		size_t block = sizes[i];

		/* memcpy needs a separate destination; skip blocks that would
		 * overlap the reserved MMU area. */
		if (block * 2 > avail)
			break;

		uint64_t t_cp = bench_memcpy(base + block, base, block);
		uint64_t t_ms = bench_memset(base, block);
		uint64_t t_rd = bench_read(base, block);
		uint64_t t_wr = bench_write(base, block);

		pr_info("%6uK   %7u   %7u   %7u   %7u   (MiB/s)\n",
			(uint32_t)(block / 1024),
			bytes_per_mib_s(block, t_cp), bytes_per_mib_s(block, t_ms),
			bytes_per_mib_s(block, t_rd), bytes_per_mib_s(block, t_wr));
	}

	pr_info("\n");

	bench_latency(base, avail);

	pr_info("\nMem speed test done.\n");
}
