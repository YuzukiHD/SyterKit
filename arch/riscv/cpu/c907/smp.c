/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file smp.c
 * @brief Secondary hart start-up for the Xuantie C907.
 */

#include <stdint.h>

#include <cache.h>
#include <csr.h>
#include <smp.h>
#include <timer.h>

extern char _start[];

uint8_t smp_stacks[SMP_MAX_HARTS][SMP_STACK_SIZE] __attribute__((aligned(16)));

struct smp_mailbox {
	volatile smp_entry_t entry;
	volatile unsigned long arg;
	volatile uint32_t started;
} __attribute__((aligned(64)));

static struct smp_mailbox smp_mailbox[SMP_MAX_HARTS];

static void smp_mailbox_clean(struct smp_mailbox *mb)
{
	flush_dcache_range((uintptr_t)mb, (uintptr_t)mb + sizeof(*mb));
}

int __attribute__((weak)) smp_soc_hart_start(unsigned int hartid, unsigned long entry_addr)
{
	(void)hartid;
	(void)entry_addr;
	return -1;
}

/* Called from _start_secondary on the new hart, with a private stack. */
void smp_secondary_main(void)
{
	unsigned int hartid = csr_read(mhartid);
	struct smp_mailbox *mb = &smp_mailbox[hartid];

	/* Same cache set-up as hart 0 (start.S). */
	cache_init();
#ifdef CONFIG_ARCH_ICACHE
	icache_enable();
#endif
#ifdef CONFIG_ARCH_DCACHE
	dcache_enable();
#endif
	asm volatile("fence.i");
	data_sync_barrier();

	/*
	 * No invalidate on either side: with msmpr set the caches are coherent,
	 * and a cache maintenance operation is broadcast, so an invalidate by
	 * the reader would discard the writer's dirty line.
	 */
	mb->started = 1;
	smp_mailbox_clean(mb);

	if (mb->entry)
		mb->entry(mb->arg);

	for (;;)
		asm volatile("wfi");
}

int smp_hart_start(unsigned int hartid, smp_entry_t entry, unsigned long arg, uint32_t timeout_ms)
{
	struct smp_mailbox *mb;
	uint32_t start;

	if (hartid == 0 || hartid >= SMP_MAX_HARTS)
		return -1;

	mb = &smp_mailbox[hartid];
	mb->entry = entry;
	mb->arg = arg;
	mb->started = 0;
	smp_mailbox_clean(mb);
	data_sync_barrier();

	if (smp_soc_hart_start(hartid, (unsigned long)_start) != 0)
		return -1;

	start = time_ms();
	while (time_ms() - start < timeout_ms) {
		if (mb->started)
			return 0;
	}
	return -2;
}
