/* SPDX-License-Identifier: GPL-2.0+ */

/*
 * Bring up the second C907 hart and run real work on both harts. Every
 * result is checked against a single hart reference. No DRAM needed.
 */

#include <drivers/serial/serial.h>

#include <common.h>
#include <csr.h>
#include <io.h>
#include <log.h>
#include <smp.h>
#include <timer.h>

#define ATOMIC_ROUNDS 1000000U
#define LOCK_ROUNDS   200000U
#define HASH_WORDS    4096U
#define PRIME_LIMIT   60000U
#define FP_TERMS      20000U

enum job_kind { JOB_NONE, JOB_ATOMIC, JOB_LOCK, JOB_HASH, JOB_PRIME, JOB_FP };

/* One field per cache line: no false sharing between the two harts. */
#define LINE __attribute__((aligned(64)))

static struct {
	volatile uint32_t seq LINE;
	volatile uint32_t done LINE;
	volatile uint32_t kind LINE;
	volatile uint32_t result LINE;
	volatile uint64_t fp_bits LINE;
	volatile uint32_t id LINE;
} job = { .id = 0xffffffffU };

static uint32_t atomic_counter LINE;
static uint32_t lock_word LINE;
static uint32_t lock_counter LINE;
static uint32_t buffer[HASH_WORDS] LINE;

static void lock(void)
{
	while (__atomic_exchange_n(&lock_word, 1, __ATOMIC_ACQUIRE))
		;
}

static void unlock(void)
{
	__atomic_store_n(&lock_word, 0, __ATOMIC_RELEASE);
}

static uint32_t hash_word(uint32_t w, uint32_t index)
{
	w ^= index * 2654435761U;
	w = (w << 13) | (w >> 19);
	return w * 0x9e3779b1U;
}

static int is_prime(uint32_t n)
{
	uint32_t d;

	if (n < 2)
		return 0;
	for (d = 2; d * d <= n; d++)
		if (n % d == 0)
			return 0;
	return 1;
}

/* Part @p of @n parts of a job; the sums add up to the single hart answer. */
static uint32_t run_part(uint32_t kind, uint32_t p, uint32_t n)
{
	uint32_t i, sum = 0;

	switch (kind) {
	case JOB_ATOMIC:
		for (i = 0; i < ATOMIC_ROUNDS; i++)
			__atomic_fetch_add(&atomic_counter, 1, __ATOMIC_RELAXED);
		break;
	case JOB_LOCK:
		for (i = 0; i < LOCK_ROUNDS; i++) {
			lock();
			lock_counter = lock_counter + 1;
			unlock();
		}
		break;
	case JOB_HASH:
		for (i = p; i < HASH_WORDS; i += n)
			sum += hash_word(buffer[i], i);
		break;
	case JOB_PRIME:
		/* 2 is prime; odd candidates are dealt out in turn so both parts weigh the same. */
		if (p == 0)
			sum += 1;
		for (i = 3 + 2 * p; i < PRIME_LIMIT; i += 2 * n)
			sum += is_prime(i);
		break;
	}
	return sum;
}

static uint64_t fp_series(void)
{
	double acc = 0.0;
	uint32_t i;
	union {
		double d;
		uint64_t u;
	} v;

	for (i = 1; i <= FP_TERMS; i++)
		acc += 1.0 / ((double)i * (double)i) - 1.0 / ((double)i * (double)i * (double)i);
	v.d = acc;
	return v.u;
}

static void hart1_main(unsigned long arg)
{
	uint32_t seen = 0;

	job.id = csr_read(mhartid);
	for (;;) {
		uint32_t seq;

		while ((seq = __atomic_load_n(&job.seq, __ATOMIC_ACQUIRE)) == seen)
			;
		seen = seq;
		if (job.kind == JOB_FP)
			job.fp_bits = fp_series();
		else
			job.result = run_part(job.kind, 1, 2);
		__atomic_store_n(&job.done, seen, __ATOMIC_RELEASE);
	}
}

/* Run the job on both harts, return hart 0 part + hart 1 part. */
static uint32_t run_both(uint32_t kind, uint32_t *us)
{
	static uint32_t seq;
	uint64_t t0 = time_us();
	uint32_t mine;

	job.kind = kind;
	__atomic_store_n(&job.seq, ++seq, __ATOMIC_RELEASE);
	mine = run_part(kind, 0, 2);
	while (__atomic_load_n(&job.done, __ATOMIC_ACQUIRE) != seq)
		;
	*us = (uint32_t)(time_us() - t0);
	return mine + job.result;
}

static void report(const char *name, int ok, uint32_t one_us, uint32_t two_us)
{
	uint32_t x100 = (uint32_t)(one_us * 100ULL / (two_us ? two_us : 1));

	pr_info("TEST %-8s %s    1 hart %u us, 2 harts %u us, x%u.%02u\n", name, ok ? "PASS" : "FAIL", one_us, two_us,
		x100 / 100, x100 % 100);
}

int main(void)
{
	uint32_t i, us, one_us, two_us, ref, got;
	uint64_t fp_local, t0;
	int failures = 0, ok;

	if (sunxi_serial_init_stdout() != 0)
		return -1;

	show_banner();
	pr_info("SMP: running on hart %u\n", (unsigned int)csr_read(mhartid));

	if (smp_hart_start(1, hart1_main, 0, 200) != 0) {
		pr_err("SMP: hart 1 did not start\n");
		return -1;
	}
	while (job.id == 0xffffffffU)
		;
	pr_info("SMP: hart %u is up\n", job.id);

	/* Atomic counter: both harts add ATOMIC_ROUNDS. */
	run_both(JOB_ATOMIC, &us);
	ok = atomic_counter == 2U * ATOMIC_ROUNDS;
	pr_info("TEST atomic   %s    counter %u, expected %u (%u us)\n", ok ? "PASS" : "FAIL", atomic_counter,
		2U * ATOMIC_ROUNDS, us);
	failures += !ok;

	/* Plain increment under a spinlock. */
	run_both(JOB_LOCK, &us);
	ok = lock_counter == 2U * LOCK_ROUNDS;
	pr_info("TEST lock     %s    counter %u, expected %u (%u us)\n", ok ? "PASS" : "FAIL", lock_counter,
		2U * LOCK_ROUNDS, us);
	failures += !ok;

	/* Hash of a buffer written by hart 0 and read by both. */
	for (i = 0; i < HASH_WORDS; i++)
		buffer[i] = i * 1664525U + 1013904223U;
	t0 = time_us();
	ref = run_part(JOB_HASH, 0, 1);
	one_us = (uint32_t)(time_us() - t0);
	got = run_both(JOB_HASH, &two_us);
	ok = got == ref;
	report("hash", ok, one_us, two_us);
	pr_info("     hash %08x, 2 harts %08x\n", ref, got);
	failures += !ok;

	/* Prime count, odd candidates dealt out in turn. */
	t0 = time_us();
	ref = run_part(JOB_PRIME, 0, 1);
	one_us = (uint32_t)(time_us() - t0);
	got = run_both(JOB_PRIME, &two_us);
	ok = got == ref;
	report("primes", ok, one_us, two_us);
	pr_info("     primes below %u: %u, 2 harts %u\n", PRIME_LIMIT, ref, got);
	failures += !ok;

	/* Floating point: hart 1 must give the same bits as hart 0. */
	fp_local = fp_series();
	run_both(JOB_FP, &us);
	ok = job.fp_bits == fp_local;
	pr_info("TEST fp       %s    hart0 %08x%08x, hart1 %08x%08x\n", ok ? "PASS" : "FAIL", (uint32_t)(fp_local >> 32),
		(uint32_t)fp_local, (uint32_t)(job.fp_bits >> 32), (uint32_t)job.fp_bits);
	failures += !ok;

	if (failures)
		pr_err("SMP: %d test(s) FAILED\n", failures);
	else
		pr_info("SMP: all tests passed (0 failures)\n");

	for (;;)
		__asm__ __volatile__("wfi");
}
