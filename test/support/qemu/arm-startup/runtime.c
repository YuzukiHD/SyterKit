/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdint.h>

#include <initcall.h>
#include <linkage.h>

void semihost_write0(const char *text);

volatile uint32_t startup_bss_probe;
volatile uint32_t startup_timer_seen;
volatile uint32_t startup_initcall_count;
volatile uint32_t startup_initcall_order;
volatile uint32_t startup_cpsr;
volatile uint32_t startup_svc_sp;
volatile uint32_t startup_und_sp;
volatile uint32_t startup_abt_sp;
volatile uint32_t startup_irq_sp;
volatile uint32_t startup_fiq_sp;
volatile uint32_t startup_vbar;
volatile uint32_t startup_sctlr;
volatile uint32_t startup_cpacr;
volatile uint32_t startup_fpexc;
volatile uint32_t startup_neon_value;

extern char _start[];
extern char __stack_und_end[];
extern char __stack_abt_end[];
extern char __stack_irq_end[];
extern char __stack_fiq_end[];
extern char __stack_srv_end[];

static int startup_test_initcall(void) {
	startup_initcall_count++;
	startup_initcall_order = startup_timer_seen;
	return 0;
}
early_initcall(startup_test_initcall);

static unsigned int failures;

static void check(int condition, const char *name) {
	if (condition)
		return;
	semihost_write0("CHECK FAIL ");
	semihost_write0(name);
	semihost_write0("\n");
	failures++;
}

static uintptr_t address(const char *symbol) {
	return (uintptr_t) symbol;
}

int main(void) {
	check(startup_bss_probe == 0U, "bss-clear");
	check(startup_timer_seen == 0x54494d45U, "timer-before-initcall");
	check(startup_initcall_count == 1U, "initcall-before-main");
	check(startup_initcall_order == 0x54494d45U, "startup-order");
	check(do_initcalls() == 0 && startup_initcall_count == 1U,
	      "initcall-once");

	check((startup_cpsr & ARMV7_MODE_MASK) == ARMV7_SVC_MODE,
	      "svc-mode");
	check((startup_cpsr & (ARMV7_IRQ_MASK | ARMV7_FIQ_MASK)) ==
		(ARMV7_IRQ_MASK | ARMV7_FIQ_MASK), "interrupt-mask");
	check((startup_cpsr & (1U << 9)) == 0U, "little-endian");
	check(startup_svc_sp == address(__stack_srv_end), "svc-stack");
	check((startup_svc_sp & 0xfU) == 0U, "stack-alignment");

#ifndef CONFIG_ARCH_MINSTACK
	check(startup_und_sp == address(__stack_und_end), "und-stack");
	check(startup_abt_sp == address(__stack_abt_end), "abt-stack");
	check(startup_irq_sp == address(__stack_irq_end), "irq-stack");
	check(startup_fiq_sp == address(__stack_fiq_end), "fiq-stack");
#endif

	check(startup_vbar == address(_start) + 0x20U, "vbar");
	check((startup_sctlr & 0x00003007U) == 0U, "mmu-cache-disable");
	check((startup_sctlr & 0x00000800U) != 0U, "branch-prediction");
	check((startup_cpacr & 0x00f00000U) == 0x00f00000U, "cpacr");
	check((startup_fpexc & 0x40000000U) != 0U, "fpexc");
	check(startup_neon_value == 0x5aU, "neon");

	if (failures == 0U)
		semihost_write0("TEST PASS arm_startup\n");
	return (int) failures;
}

void arm32_do_undefined_instruction(void *regs) {
	(void) regs;
}

void arm32_do_software_interrupt(void *regs) {
	(void) regs;
}

void arm32_do_prefetch_abort(void *regs) {
	(void) regs;
}

void arm32_do_data_abort(void *regs) {
	(void) regs;
}

void arm32_do_irq(void *regs) {
	(void) regs;
}

void arm32_do_fiq(void *regs) {
	(void) regs;
}
