/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file backtrace-min.c
 * @brief Minimal address-only backtrace printer.
 *
 * This implementation is selected when generated ELF symbol tables are not
 * available.  It preserves the common backtrace interface and prints raw
 * program-counter values without attempting stack unwinding.
 */

#include <stdint.h>

#include <backtrace.h>
#include <log.h>

/** @brief Begin a minimal raw-address call trace. */
void backtrace_print_begin(void)
{
}

/**
 * @brief Print one raw program-counter address.
 * @param[in] address Address captured from the call stack.
 */
void backtrace_print_frame(uintptr_t address)
{
#if __SIZEOF_POINTER__ == 8
	printk(LOG_LEVEL_BACKTRACE, "backtrace: 0x%016lx\n", (unsigned long)address);
#else
	printk(LOG_LEVEL_BACKTRACE, "backtrace: 0x%08x\n", (uint32_t)address);
#endif
}

/** @brief Finish a minimal raw-address call trace. */
void backtrace_print_end(void)
{
}
