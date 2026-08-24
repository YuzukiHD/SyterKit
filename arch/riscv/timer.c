/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdint.h>

#include <timer.h>

#define DEFAULT_TIMER_FREQ_MHZ 24U

static uint32_t init_timestamp;
uint32_t current_hosc_freq = DEFAULT_TIMER_FREQ_MHZ;

int __attribute__((weak)) sunxi_hosc_detect(void)
{
	return DEFAULT_TIMER_FREQ_MHZ;
}

static uint32_t timer_freq_mhz(void)
{
	return current_hosc_freq ? current_hosc_freq : DEFAULT_TIMER_FREQ_MHZ;
}

void set_timer_count(void)
{
	int detected_freq = sunxi_hosc_detect();

	if (detected_freq > 0)
		current_hosc_freq = (uint32_t)detected_freq;
	init_timestamp = (uint32_t)time_us();
}

uint64_t get_arch_counter(void)
{
#if __riscv_xlen == 32
	uint32_t upper, lower, upper_new;

	asm volatile("1:  rdtimeh %[upper]\n"
		     "    rdtime %[lower]\n"
		     "    rdtimeh %[upper_new]\n"
		     "    bne %[upper], %[upper_new], 1b\n"
		     : [upper] "=r"(upper), [lower] "=r"(lower), [upper_new] "=&r"(upper_new)
		     :
		     : "memory");

	return ((uint64_t)upper << 32) | lower;
#else
	uint64_t counter;

	asm volatile("csrr %0, time" : "=r"(counter) : : "memory");
	return counter;
#endif
}

uint32_t time_ms(void)
{
	return (uint32_t)(get_arch_counter() / ((uint64_t)timer_freq_mhz() * 1000U));
}

uint64_t time_us(void)
{
	return get_arch_counter() / timer_freq_mhz();
}

void udelay(uint32_t us)
{
	uint64_t start = get_arch_counter();
	uint64_t ticks = (uint64_t)us * timer_freq_mhz();

	while (get_arch_counter() - start < ticks)
		;
}

void mdelay(uint32_t ms)
{
	udelay(ms * 1000U);
}

void sdelay(uint32_t loops)
{
#if __riscv_xlen == 32
	udelay(loops);
#else
	if (!loops)
		return;
	asm volatile("mv t0, %0\n"
		     "1:\n"
		     "addi t0, t0, -1\n"
		     "bnez t0, 1b\n"
		     :
		     : "r"(loops)
		     : "t0");
#endif
}

uint32_t get_init_timestamp(void)
{
	return init_timestamp;
}
