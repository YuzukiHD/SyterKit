/* SPDX-License-Identifier: GPL-2.0+ */

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

void backtrace_print_begin(void)
{
	printk(LOG_LEVEL_BACKTRACE, "Call trace:\n");
}

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

void backtrace_print_end(void)
{
}
