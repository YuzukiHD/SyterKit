/* SPDX-License-Identifier: GPL-2.0+ */

#ifndef __BACKTRACE_H__
#define __BACKTRACE_H__

#include <stdint.h>

/**
 * @brief Saved execution context used to start an architecture unwind.
 */
struct backtrace_context {
	uintptr_t pc; /**< Program counter at which unwinding starts. */
	uintptr_t sp; /**< Stack pointer at which unwinding starts. */
	uintptr_t fp; /**< Frame pointer when required by the architecture. */
	uintptr_t lr; /**< Link register associated with the saved context. */
};

#ifdef CONFIG_BACKTRACE
/**
 * @brief Walk a saved execution context and print its call trace.
 * @param[in] pc Program counter at which unwinding starts.
 * @param[in] sp Stack pointer at which unwinding starts.
 * @param[in] lr Link register associated with the saved context.
 * @return Number of frames printed, or zero when the context is invalid.
 */
int backtrace(char *pc, long *sp, char *lr);

/**
 * @brief Walk a saved frame-pointer context and print its call trace.
 * @param[in] context Saved execution context.
 * @return Number of frames printed, or zero when the context is invalid.
 */
int backtrace_from_context(const struct backtrace_context *context);

/**
 * @brief Capture the caller context and print its call trace.
 * @return Number of frames printed, or zero when the context is invalid.
 */
int dump_stack(void);

/**
 * @brief Start formatting one call trace.
 */
void backtrace_print_begin(void);

/**
 * @brief Format one program counter in the current call trace.
 * @param[in] address Program-counter address to print.
 */
void backtrace_print_frame(uintptr_t address);

/**
 * @brief Finish formatting one call trace.
 */
void backtrace_print_end(void);

#else /* CONFIG_BACKTRACE */

#define backtrace(pc, sp, lr) \
	do {                  \
	} while (0)

#define backtrace_from_context(context) \
	do {                            \
	} while (0)

#define dump_stack() \
	do {         \
	} while (0)

#define backtrace_print_begin() \
	do {                    \
	} while (0)

#define backtrace_print_frame(address) \
	do {                           \
	} while (0)

#define backtrace_print_end() \
	do {                  \
	} while (0)

#endif /* CONFIG_BACKTRACE */

#endif
