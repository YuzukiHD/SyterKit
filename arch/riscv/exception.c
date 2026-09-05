/* SPDX-License-Identifier: GPL-2.0+ */

#include <stdbool.h>
#include <stdint.h>

#if defined(CONFIG_BACKTRACE)
#include <backtrace.h>
#endif
#include <log.h>

#if defined(CONFIG_DRIVER_INTC)
#include <drivers/intc/intc.h>
#endif

#define RISCV_CAUSE_INTERRUPT (1UL << (__riscv_xlen - 1))

/* Must match the frame built by SAVE_ALL in arch/riscv/include/asm/trap.h. */
struct pt_regs_t {
	unsigned long x[32];
	unsigned long status;
	unsigned long epc;
	unsigned long badvaddr;
	unsigned long cause;
	unsigned long insn;
};

static const char *riscv_trap_name(unsigned long cause, bool interrupt)
{
	if (interrupt) {
		switch (cause) {
		case 0U:
			return "user software interrupt";
		case 1U:
			return "supervisor software interrupt";
		case 3U:
			return "machine software interrupt";
		case 4U:
			return "user timer interrupt";
		case 5U:
			return "supervisor timer interrupt";
		case 7U:
			return "machine timer interrupt";
		case 8U:
			return "user external interrupt";
		case 9U:
			return "supervisor external interrupt";
		case 11U:
			return "machine external interrupt";
		default:
			return "unknown interrupt";
		}
	}

	switch (cause) {
	case 0U:
		return "instruction address misaligned";
	case 1U:
		return "instruction access fault";
	case 2U:
		return "illegal instruction";
	case 3U:
		return "breakpoint";
	case 4U:
		return "load address misaligned";
	case 5U:
		return "load access fault";
	case 6U:
		return "store address misaligned";
	case 7U:
		return "store access fault";
	case 8U:
		return "environment call from U-mode";
	case 9U:
		return "environment call from S-mode";
	case 11U:
		return "environment call from M-mode";
	case 12U:
		return "instruction page fault";
	case 13U:
		return "load page fault";
	case 15U:
		return "store page fault";
	default:
		return "unknown exception";
	}
}

static void riscv_show_trap(const struct pt_regs_t *regs)
{
	bool interrupt = (regs->cause & RISCV_CAUSE_INTERRUPT) != 0U;
	unsigned long cause = regs->cause & ~RISCV_CAUSE_INTERRUPT;

	pr_err("RISC-V %s: %s (%lu)\n", interrupt ? "interrupt" : "exception", riscv_trap_name(cause, interrupt), cause);
	pr_err("mcause=0x%08lx mepc=0x%08lx mtval=0x%08lx mstatus=0x%08lx\n", regs->cause, regs->epc, regs->badvaddr, regs->status);
	pr_err("ra=0x%08lx sp=0x%08lx gp=0x%08lx tp=0x%08lx\n", regs->x[1], regs->x[2], regs->x[3], regs->x[4]);
	pr_err("t0=0x%08lx t1=0x%08lx t2=0x%08lx s0=0x%08lx\n", regs->x[5], regs->x[6], regs->x[7], regs->x[8]);
	pr_err("s1=0x%08lx s2=0x%08lx s3=0x%08lx s4=0x%08lx\n", regs->x[9], regs->x[18], regs->x[19], regs->x[20]);
	pr_err("s5=0x%08lx s6=0x%08lx s7=0x%08lx s8=0x%08lx\n", regs->x[21], regs->x[22], regs->x[23], regs->x[24]);
	pr_err("s9=0x%08lx s10=0x%08lx s11=0x%08lx t3=0x%08lx\n", regs->x[25], regs->x[26], regs->x[27], regs->x[28]);
	pr_err("t4=0x%08lx t5=0x%08lx t6=0x%08lx\n", regs->x[29], regs->x[30], regs->x[31]);
	pr_err("a0=0x%08lx a1=0x%08lx a2=0x%08lx a3=0x%08lx\n", regs->x[10], regs->x[11], regs->x[12], regs->x[13]);
	pr_err("a4=0x%08lx a5=0x%08lx a6=0x%08lx a7=0x%08lx\n", regs->x[14], regs->x[15], regs->x[16], regs->x[17]);

#if defined(CONFIG_BACKTRACE)
	{
		const struct backtrace_context context = {
			.pc = regs->epc,
			.sp = regs->x[2],
			.fp = regs->x[8],
			.lr = regs->x[1],
		};

		backtrace_from_context(&context);
	}
#endif
}

static __attribute__((noreturn)) void riscv_trap_halt(void)
{
	for (;;)
		__asm__ __volatile__("wfi");
}

void riscv_handle_exception(struct pt_regs_t *regs)
{
	unsigned long cause;

	if ((regs->cause & RISCV_CAUSE_INTERRUPT) == 0U) {
		riscv_show_trap(regs);
		riscv_trap_halt();
	}

	cause = regs->cause & ~RISCV_CAUSE_INTERRUPT;
#if defined(CONFIG_DRIVER_INTC)
	if (intc_handle_irq(cause))
		return;
#else
	(void)cause;
#endif

	riscv_show_trap(regs);
	riscv_trap_halt();
}
