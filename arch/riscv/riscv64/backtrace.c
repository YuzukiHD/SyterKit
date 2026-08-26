/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file backtrace.c
 * @brief Frame-pointer call-trace unwinder for RV64 targets.
 *
 * The implementation validates both image and stack bounds before following
 * the frame chain and falls back to a saved link register when no frame is
 * available.
 */

#include <stddef.h>
#include <stdint.h>

#include <backtrace.h>

/** @brief Maximum number of frames emitted by one backtrace. */
#define BACKTRACE_LEVEL_LIMIT 64U

/**
 * @brief RISC-V frame record stored immediately below the frame pointer.
 */
struct stackframe {
	uintptr_t fp; /**< Previous frame pointer. */
	uintptr_t ra; /**< Saved return address. */
};

extern uint8_t __spl_start[];
extern uint8_t __spl_end[];
extern uint8_t __stack_srv_start[];
extern uint8_t __stack_srv_end[];

/**
 * @brief Check whether a program counter belongs to the current image.
 * @param[in] pc Program-counter value.
 * @return Nonzero when the address belongs to executable image storage.
 */
static int backtrace_pc_valid(uintptr_t pc)
{
	uintptr_t start = (uintptr_t)__spl_start;
	uintptr_t end = (uintptr_t)__spl_end;

	return pc >= start && pc < end;
}

/**
 * @brief Check whether a stack pointer belongs to the service stack.
 * @param[in] sp Stack-pointer value.
 * @return Nonzero when at least one frame record can follow the pointer.
 */
static int backtrace_sp_valid(uintptr_t sp)
{
	uintptr_t start = (uintptr_t)__stack_srv_start;
	uintptr_t end = (uintptr_t)__stack_srv_end;

	return sp >= start && sp <= end - sizeof(struct stackframe);
}

/**
 * @brief Validate one RISC-V frame pointer before dereferencing it.
 * @param[in] sp Stack pointer for the current frame.
 * @param[in] fp Frame pointer for the current frame.
 * @return Nonzero when the complete frame record is readable.
 */
static int backtrace_frame_valid(uintptr_t sp, uintptr_t fp)
{
	uintptr_t end = (uintptr_t)__stack_srv_end;

	if (!backtrace_sp_valid(sp))
		return 0;
	if (fp & (sizeof(uintptr_t) - 1U))
		return 0;
	return fp >= sp + sizeof(struct stackframe) && fp <= end;
}

/**
 * @brief Walk a frame-pointer context and print its RISC-V call trace.
 * @param[in] context Saved PC, stack, frame, and link registers.
 * @return Number of frames printed, or zero when the context is invalid.
 */
int backtrace_from_context(const struct backtrace_context *context)
{
	uintptr_t fp;
	uintptr_t pc;
	uintptr_t sp;
	unsigned int level;

	if (!context || !backtrace_pc_valid(context->pc) || !backtrace_sp_valid(context->sp))
		return 0;

	pc = context->pc;
	sp = context->sp;
	fp = context->fp;
	backtrace_print_begin();
	backtrace_print_frame(pc);
	level = 1U;

	while (level < BACKTRACE_LEVEL_LIMIT && backtrace_frame_valid(sp, fp)) {
		const struct stackframe *frame = (const struct stackframe *)fp - 1;

		sp = fp;
		fp = frame->fp;
		pc = frame->ra;
		if (!backtrace_pc_valid(pc))
			break;
		backtrace_print_frame(pc);
		level++;
	}

	if (level == 1U && context->lr != pc && backtrace_pc_valid(context->lr)) {
		backtrace_print_frame(context->lr);
		level++;
	}
	backtrace_print_end();
	return (int)level;
}

/**
 * @brief Adapt raw register values to a context and print a call trace.
 * @param[in] pc Starting program counter.
 * @param[in] sp Starting stack pointer.
 * @param[in] lr Link register used when no frame chain is available.
 * @return Number of frames printed, or zero for invalid addresses.
 */
int backtrace(char *pc, long *sp, char *lr)
{
	const struct backtrace_context context = {
		.pc = (uintptr_t)pc,
		.sp = (uintptr_t)sp,
		.fp = 0U,
		.lr = (uintptr_t)lr,
	};

	return backtrace_from_context(&context);
}

/**
 * @brief Print a trace from registers captured by the naked entry point.
 * @param[in] sp Captured stack pointer.
 * @param[in] lr Captured return address.
 * @param[in] fp Captured frame pointer.
 * @return Number of frames printed.
 */
static int __attribute__((noinline, used)) dump_stack_from_context(uintptr_t sp, uintptr_t lr, uintptr_t fp)
{
	const struct backtrace_context context = {
		.pc = lr,
		.sp = sp,
		.fp = fp,
		.lr = lr,
	};

	return backtrace_from_context(&context);
}

/**
 * @brief Capture the current RISC-V registers and print a call trace.
 * @return Number of frames printed by the frame-pointer unwinder.
 */
int __attribute__((naked)) dump_stack(void)
{
	asm volatile("mv a0, sp\n"
		     "mv a1, ra\n"
		     "mv a2, s0\n"
		     "tail dump_stack_from_context\n");
}
