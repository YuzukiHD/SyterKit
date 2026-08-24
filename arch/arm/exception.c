/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file exception.c
 * @brief ARM exception diagnostics and weak board hooks.
 *
 * Default handlers print the saved register frame, optionally emit a
 * backtrace, advance past the faulting instruction where required, and stop
 * through the common abort path.  Boards can override any handler.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <types.h>

#include <mmu.h>
#include <timer.h>

#if defined(CONFIG_BACKTRACE)
#include <backtrace.h>
#endif
#include <log.h>

extern void abort();

// #define START_UP_DEBUG
#ifdef START_UP_DEBUG
/**
 * @brief Convert an integer to a fixed-width uppercase hexadecimal string.
 *
 * The output always contains eight hexadecimal digits followed by a NUL byte;
 * the caller must provide a buffer of at least nine bytes. This diagnostic
 * helper is compiled only for START_UP_DEBUG builds.
 *
 * @param[in] value Integer value whose low 32 bits are formatted.
 * @param[out] buffer Destination buffer receiving the NUL-terminated text.
 */
void int_to_hex_string(int value, char *buffer)
{
	char hex_digits[] = "0123456789ABCDEF";
	int i;

	for (i = 0; i < 8; i++) {
		buffer[7 - i] = hex_digits[value & 0xF];
		value >>= 4;
	}
	buffer[8] = '\0';
}
#endif

/**
 * @brief Print an ARM exception register frame and optional backtrace.
 * @param[in] regs Saved register frame supplied by the exception entry stub.
 */
static void show_regs(struct arm_regs_t *regs)
{
	int i = 0;

	printk_error("pc : [<0x%08lx>] lr : [<0x%08lx>] cpsr: 0x%08lx\n", regs->pc, regs->lr, regs->cpsr);
	printk_error("sp : 0x%08lx esp : 0x%08lx\n", regs->sp, regs->esp);
	for (i = 12; i >= 0; i--)
		printk_error("r%-2d: 0x%08lx\n", i, regs->r[i]);
	printk_error("\n");

#if defined(CONFIG_BACKTRACE)
	char *pc = (char *)regs->pc;
	long *sp = (long *)regs->sp;
	char *lr = (char *)regs->lr;

	if (regs->cpsr & 0x20)
		MAKE_THUMB_ADDR(pc);
	backtrace(pc, sp, lr);
#endif
}

/**
 * @brief Handle an undefined ARM instruction.
 * @param[in,out] regs Saved frame; the PC is advanced before aborting.
 * @note Weak default hook; a board may replace it with recovery logic.
 */
void __attribute__((weak)) arm32_do_undefined_instruction(struct arm_regs_t *regs)
{
	printk_error("undefined_instruction\n");
	show_regs(regs);
	regs->pc += 4;
	abort();
}

/**
 * @brief Handle an ARM software interrupt exception.
 * @param[in,out] regs Saved frame whose PC is advanced before aborting.
 * @note Weak default hook; a board may replace it with a syscall dispatcher.
 */
void __attribute__((weak)) arm32_do_software_interrupt(struct arm_regs_t *regs)
{
	printk_error("software_interrupt\n");
	show_regs(regs);
	regs->pc += 4;
	abort();
}

/**
 * @brief Handle an instruction prefetch abort.
 * @param[in,out] regs Saved frame printed before the default abort path.
 * @note Weak default hook intended for board-specific fault recovery.
 */
void __attribute__((weak)) arm32_do_prefetch_abort(struct arm_regs_t *regs)
{
	printk_error("prefetch_abort\n");
	show_regs(regs);
	regs->pc += 4;
	abort();
}

/**
 * @brief Handle a data access abort.
 * @param[in,out] regs Saved frame whose PC is advanced before aborting.
 * @note Weak default hook intended for board-specific fault recovery.
 */
void __attribute__((weak)) arm32_do_data_abort(struct arm_regs_t *regs)
{
	printk_error("data_abort\n");
	show_regs(regs);
	regs->pc += 4;
	abort();
}

/**
 * @brief Handle an unexpected ARM IRQ.
 * @param[in] regs Saved register frame to print before aborting.
 * @note Weak default hook for boards that provide an interrupt controller.
 */
void __attribute__((weak)) arm32_do_irq(struct arm_regs_t *regs)
{
	printk_error("undefined IRQ\n");
	show_regs(regs);
	abort();
}

/**
 * @brief Handle an unexpected ARM FIQ.
 * @param[in] regs Saved register frame to print before aborting.
 * @note Weak default hook for boards that provide a fast interrupt handler.
 */
void __attribute__((weak)) arm32_do_fiq(struct arm_regs_t *regs)
{
	printk_error("undefined FIQ\n");
	show_regs(regs);
	abort();
}
