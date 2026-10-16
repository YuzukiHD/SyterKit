/* SPDX-License-Identifier: GPL-2.0+ */

/*
 * Run loop of memtester 4.6.0 (memtester.c, Charles Cazabon, GPL-2.0) for a fixed region
 * of memory: no mmap/mlock, the buffer is given by the caller.
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <log.h>
#include <timer.h>
#include <uart.h>

#include "../types.h"
#include "../sizes.h"
#include "../tests.h"
#include "../memtester.h"

#define EXIT_FAIL_ADDRESSLINES 0x02
#define EXIT_FAIL_OTHERTEST    0x04

int use_phys;
off_t physaddrbase;

struct test tests[] = {
	{ "Random Value", test_random_value },
	{ "Compare XOR", test_xor_comparison },
	{ "Compare SUB", test_sub_comparison },
	{ "Compare MUL", test_mul_comparison },
	{ "Compare DIV", test_div_comparison },
	{ "Compare OR", test_or_comparison },
	{ "Compare AND", test_and_comparison },
	{ "Sequential Increment", test_seqinc_comparison },
	{ "Solid Bits", test_solidbits_comparison },
	{ "Block Sequential", test_blockseq_comparison },
	{ "Checkerboard", test_checkerboard_comparison },
	{ "Bit Spread", test_bitspread_comparison },
	{ "Bit Flip", test_bitflip_comparison },
	{ "Walking Ones", test_walkbits1_comparison },
	{ "Walking Zeroes", test_walkbits0_comparison },
	{ NULL, NULL }
};

int putchar(int c)
{
	if (c == '\n')
		uart_putchar('\r');
	return uart_putchar(c);
}

/* xorshift32, 31 bits like the usual rand(); rand32() of memtester joins two calls */
int rand(void)
{
	static uint32_t state = 1;

	state ^= state << 13;
	state ^= state >> 17;
	state ^= state << 5;
	return (int)(state >> 1);
}

/*
 * Test @p bytes at @p base, @p loops times (0: for ever). The region is split in two halves,
 * the tests compare one against the other. Returns 0, or the OR of the failure bits.
 */
int memtester_run(void *base, size_t bytes, unsigned int loops)
{
	ulv *bufa = (ulv *)base;
	ulv *bufb = (ulv *)((size_t)base + bytes / 2);
	size_t count = bytes / 2 / sizeof(ul);
	unsigned int loop, i;
	int exit_code = 0;

	pr_info("memtester: %u KiB at 0x%08x, %u loop(s)\n", (unsigned int)(bytes >> 10), (unsigned int)base, loops);
	for (loop = 1; !loops || loop <= loops; loop++) {
		uint32_t t0 = time_ms();

		printf("Loop %u", loop);
		if (loops)
			printf("/%u", loops);
		printf(":\n  %-20s: ", "Stuck Address");
		if (!test_stuck_address(bufa, bytes / sizeof(ul)))
			printf("ok (%u ms)\n", time_ms() - t0);
		else
			exit_code |= EXIT_FAIL_ADDRESSLINES;
		for (i = 0; tests[i].name; i++) {
			t0 = time_ms();
			printf("  %-20s: ", tests[i].name);
			if (!tests[i].fp(bufa, bufb, count))
				printf("ok (%u ms)\n", time_ms() - t0);
			else
				exit_code |= EXIT_FAIL_OTHERTEST;
			memset((void *)base, 255, bytes);
		}
		printf("\n");
	}
	printf("Done.\n");
	return exit_code;
}
