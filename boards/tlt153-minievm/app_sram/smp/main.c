/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdint.h>

#include <io.h>
#include <log.h>
#include <common.h>
#include <drivers/serial/serial.h>

#define SUNXI_SMP_CORE_COUNT 4U
#define SUNXI_SMP_STACK_SIZE 4096U

#define SUNXI_CPUS_CFG_BASE	0x07050000U
#define SUNXI_CPU_SOFT_ENTRY(c) (SUNXI_CPUS_CFG_BASE + 0x70U + (uint32_t)(c) * 4U)

#define SUNXI_CPUIDLE_BASE	0x07051000U
#define SUNXI_PWRS_STAT(c)	(SUNXI_CPUIDLE_BASE + 0x50U + (uint32_t)(c) * 4U)
#define SUNXI_CPU_CFG(c)	(SUNXI_CPUIDLE_BASE + 0x70U + (uint32_t)(c) * 4U)
#define SUNXI_HOTPLUG_CTRL(c)	(SUNXI_CPUIDLE_BASE + 0x80U + (uint32_t)(c) * 4U)
#define SUNXI_CPUIDLE_EN	(SUNXI_CPUIDLE_BASE + 0x100U)
#define SUNXI_PWR_SW_DELAY	(SUNXI_CPUIDLE_BASE + 0x140U)
#define SUNXI_F1F2_CONFIG_DELAY (SUNXI_CPUIDLE_BASE + 0x144U)

#define SUNXI_POWER_OFF	   0xffU
#define SUNXI_HOTPLUG_REQ  (1U << 16)
#define SUNXI_WAKEUP_MASK  (1U << 0)
#define SUNXI_CPU_IRQ_MASK (1U << 4)

#define SUNXI_BOOT_TIMEOUT 0x100000U
#define SUNXI_WORK_TIMEOUT 0x1000000U

static inline void smp_delay(unsigned int count)
{
	volatile unsigned int remaining = count;

	while (remaining-- != 0U)
		;
}

/* These objects are shared by CPU0 and the secondary CPUs. */
static uint8_t smp_secondary_stacks[SUNXI_SMP_CORE_COUNT][SUNXI_SMP_STACK_SIZE] __attribute__((aligned(64)));
uint32_t smp_secondary_stack_tops[SUNXI_SMP_CORE_COUNT];
static volatile uint32_t smp_secondary_alive[SUNXI_SMP_CORE_COUNT];
static void (*volatile smp_secondary_jobs[SUNXI_SMP_CORE_COUNT])(void);

static void smp_init(void)
{
	unsigned int cpu;

	for (cpu = 0; cpu < SUNXI_SMP_CORE_COUNT; cpu++)
		setbits_le32(SUNXI_CPU_CFG(cpu), SUNXI_CPU_IRQ_MASK);

	write32(SUNXI_F1F2_CONFIG_DELAY, 24U);
	write32(SUNXI_PWR_SW_DELAY, 24U);
	write32(SUNXI_CPUIDLE_EN, 0x16aa0000U);
	write32(SUNXI_CPUIDLE_EN, 0xaa160001U);

	for (cpu = 1; cpu < SUNXI_SMP_CORE_COUNT; cpu++)
		smp_secondary_stack_tops[cpu] = (uint32_t)(uintptr_t)smp_secondary_stacks[cpu] + SUNXI_SMP_STACK_SIZE;

	__asm__ volatile("dsb" ::: "memory");
}

static int smp_start_cpu(unsigned int cpu, uintptr_t entry)
{
	unsigned int timeout;

	if (cpu == 0U || cpu >= SUNXI_SMP_CORE_COUNT || entry == 0U)
		return -1;

	write32(SUNXI_CPU_SOFT_ENTRY(cpu), (uint32_t)entry);
	setbits_le32(SUNXI_HOTPLUG_CTRL(cpu), SUNXI_WAKEUP_MASK);

	/* The firmware expects a powered-off secondary before asserting wakeup. */
	if ((read32(SUNXI_PWRS_STAT(cpu)) & 0xffU) != SUNXI_POWER_OFF)
		return -2;

	smp_delay(1000U);
	setbits_le32(SUNXI_HOTPLUG_CTRL(cpu), SUNXI_HOTPLUG_REQ);

	for (timeout = 0; timeout < SUNXI_BOOT_TIMEOUT; timeout++) {
		if ((read32(SUNXI_HOTPLUG_CTRL(cpu)) & SUNXI_HOTPLUG_REQ) == 0U)
			return 0;
		smp_delay(100U);
	}

	return -3;
}

static int smp_cpu_alive(unsigned int cpu)
{
	if (cpu == 0U || cpu >= SUNXI_SMP_CORE_COUNT)
		return -1;

	return smp_secondary_alive[cpu] != 0U;
}

static void smp_submit(unsigned int cpu, void (*job)(void))
{
	if (cpu == 0U || cpu >= SUNXI_SMP_CORE_COUNT || job == 0)
		return;

	smp_secondary_jobs[cpu] = job;
	__asm__ volatile("dsb" ::: "memory");
	__asm__ volatile("sev");
}

void smp_secondary_main(unsigned int cpu)
{
	smp_secondary_alive[cpu] = 1U;
	__asm__ volatile("dsb" ::: "memory");
	clrbits_le32(SUNXI_HOTPLUG_CTRL(cpu), SUNXI_WAKEUP_MASK);

	for (;;) {
		/* SEV from CPU0 wakes a secondary through the event register. */
		__asm__ volatile("wfe");
		void (*job)(void) = smp_secondary_jobs[cpu];

		if (job != 0) {
			smp_secondary_jobs[cpu] = 0;
			job();
		}
	}
}

/* CPU_SOFT_ENTRY points here. The reset stub enters C with a private stack. */
void smp_secondary_entry(void);

static volatile uint32_t smp_work_counters[SUNXI_SMP_CORE_COUNT];

static void smp_cpu1_work(void)
{
	smp_work_counters[1]++;
}

static void smp_cpu2_work(void)
{
	smp_work_counters[2]++;
}

static void smp_cpu3_work(void)
{
	smp_work_counters[3]++;
}

static int smp_wait_for_alive(unsigned int cpu)
{
	volatile uint32_t timeout = SUNXI_WORK_TIMEOUT;

	while (timeout-- != 0U) {
		if (smp_cpu_alive(cpu) == 1)
			return 0;
	}

	return -1;
}

static int smp_wait_for_work(unsigned int cpu)
{
	volatile uint32_t timeout = SUNXI_WORK_TIMEOUT;

	while (timeout-- != 0U) {
		if (smp_work_counters[cpu] != 0U)
			return 0;
	}

	return -1;
}

int main(void)
{
	unsigned int cpu;
	unsigned int active_mask = 0U;
	int ret;

	if (sunxi_serial_init_stdout() != 0)
		return -1;

    show_banner();

	pr_info("T153 SMP application: starting CPU0 + CPU1..CPU3\n");
	smp_init();

	for (cpu = 1; cpu < SUNXI_SMP_CORE_COUNT; cpu++) {
		ret = smp_start_cpu(cpu, (uintptr_t)&smp_secondary_entry);
		if (ret != 0) {
			pr_err("SMP: CPU%u boot failed (%d)\n", cpu, ret);
			continue;
		}

		if (smp_wait_for_alive(cpu) != 0) {
			pr_err("SMP: CPU%u did not enter secondary C code\n", cpu);
			continue;
		}

		pr_info("SMP: CPU%u is alive\n", cpu);
		active_mask |= 1U << cpu;
	}

	/* Submit one callback to each live secondary. */
	if ((active_mask & (1U << 1)) != 0U)
		smp_submit(1, smp_cpu1_work);
	if ((active_mask & (1U << 2)) != 0U)
		smp_submit(2, smp_cpu2_work);
	if ((active_mask & (1U << 3)) != 0U)
		smp_submit(3, smp_cpu3_work);

	for (cpu = 1; cpu < SUNXI_SMP_CORE_COUNT; cpu++) {
		if ((active_mask & (1U << cpu)) == 0U)
			continue;
		if (smp_wait_for_work(cpu) == 0)
			pr_info("SMP: CPU%u completed its work\n", cpu);
		else
			pr_err("SMP: CPU%u did not run its work\n", cpu);
	}

	pr_info("T153 SMP application is running; secondary CPUs remain in WFI\n");
	for (;;) {
		__asm__ volatile("wfi");
	}
}
