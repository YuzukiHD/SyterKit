/* SPDX-License-Identifier: GPL-2.0+ */

/**
 * @file backtrace-full.c
 * @brief Symbol-table-backed call-trace output for full firmware builds.
 *
 * The linker supplies sorted symbol and name tables. This module resolves
 * program counters with a bounded binary search and formats unknown addresses
 * explicitly when no generated symbol covers them.
 */

#include <stddef.h>
#include <stdint.h>

#include <backtrace.h>
#include <log.h>

/**
 * @brief Compact function-symbol record generated for one output ELF.
 */
struct backtrace_symbol {
	uintptr_t address; /**< Function start address. */
	uintptr_t size; /**< Function size in bytes. */
	uintptr_t name_offset; /**< Offset in the generated name table. */
};

/** @brief Generated function-symbol records. */
extern const struct backtrace_symbol __backtrace_symbols[] __attribute__((weak));
/** @brief Number of generated function-symbol records. */
extern const uintptr_t __backtrace_symbol_count __attribute__((weak));
/** @brief Generated NUL-terminated function names. */
extern const char __backtrace_symbol_names[] __attribute__((weak));

/**
 * @brief Find the generated symbol containing an address.
 * @param[in] address Normalized program-counter address.
 * @param[out] size Resolved function size.
 * @return Matching symbol, or NULL when the address is not symbolized.
 */
static const struct backtrace_symbol *backtrace_find_symbol(uintptr_t address, uintptr_t *size)
{
	const struct backtrace_symbol *symbol;
	size_t left = 0;
	size_t right;
	size_t count;

	if (!__backtrace_symbols || !&__backtrace_symbol_count || !__backtrace_symbol_names)
		return NULL;

	count = __backtrace_symbol_count;
	if (!count)
		return NULL;
	right = count;
	while (left < right) {
		size_t middle = left + (right - left) / 2U;

		if (__backtrace_symbols[middle].address <= address)
			left = middle + 1U;
		else
			right = middle;
	}
	if (!left)
		return NULL;

	symbol = &__backtrace_symbols[left - 1U];
	*size = symbol->size;
	if (!*size && left < count)
		*size = __backtrace_symbols[left].address - symbol->address;
	if (!*size || address - symbol->address >= *size)
		return NULL;

	return symbol;
}

/**
 * @brief Begin a symbolized call-trace listing.
 *
 * The function emits the common heading used by both symbolized and
 * architecture-specific unwinders.  It does not change unwind state.
 */
void backtrace_print_begin(void)
{
	printk(LOG_LEVEL_BACKTRACE, "Call trace:\n");
}

/**
 * @brief Print one program-counter address and its nearest generated symbol.
 *
 * ARM addresses have their Thumb-state bit removed before symbol lookup.
 * Addresses outside the generated symbol table are printed with an explicit
 * `unknown` marker rather than being discarded.
 *
 * @param[in] raw_address Program-counter value supplied by the unwinder.
 */
void backtrace_print_frame(uintptr_t raw_address)
{
	const struct backtrace_symbol *symbol;
	uintptr_t address = raw_address;
	uintptr_t size = 0;

#if defined(CONFIG_ARCH_ARM32)
	address &= ~1U;
#endif
	symbol = backtrace_find_symbol(address, &size);
	if (!symbol) {
#if __SIZEOF_POINTER__ == 8
		printk(LOG_LEVEL_BACKTRACE, " [<0x%016lx>] <unknown>\n", (unsigned long)address);
#else
		printk(LOG_LEVEL_BACKTRACE, " [<0x%08x>] <unknown>\n", (uint32_t)address);
#endif
		return;
	}

#if __SIZEOF_POINTER__ == 8
	printk(LOG_LEVEL_BACKTRACE, " [<0x%016lx>] %s+0x%lx/0x%lx\n", (unsigned long)address, __backtrace_symbol_names + symbol->name_offset,
	       (unsigned long)(address - symbol->address), (unsigned long)size);
#else
	printk(LOG_LEVEL_BACKTRACE, " [<0x%08x>] %s+0x%x/0x%x\n", (uint32_t)address, __backtrace_symbol_names + symbol->name_offset, (uint32_t)(address - symbol->address),
	       (uint32_t)size);
#endif
}

/**
 * @brief Finish a symbolized call-trace listing.
 *
 * The full-symbol backend currently needs no trailer, but this hook keeps
 * output framing consistent with the architecture-specific backtrace code.
 */
void backtrace_print_end(void)
{
}
